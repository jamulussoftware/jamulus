#!/usr/bin/env python3
"""
Generates a list of Github user names who contributed to a specific release.

May need a Github Personal access token to avoid hitting rate limits.

Usage:

## Website repo (jamulussoftware/jamuluswebsite)
- cd jamuluswebsite
- git log --oneline release # Find commit ID of previous version. Search for "RELEASE:"
- /path/to/jamulussoftware/code/tools/get_release_contributors.py --from COMMIT_ID_FROM_PREVIOUS_STEP --to next-release --github-token YOUR_TOKEN --repo .

## Code repo (jamulussoftware/jamulus)
- git pull -C /path/
- ./tools/get_release_contributors.py --from r3_8_1 --to r3_8_2 --github-token YOUR_TOKEN --repo .

"""
import requests
import argparse
import logging
import os
import re
import subprocess
import yaml

logger = logging.getLogger('')


class UnexpectedGithubStatus(RuntimeError):
    """
    Exception which is raised when an API request fails.
    """
    pass


# List of user names which should be ignored (such as bots):
ignore_list = ['github-actions[bot]', 'imgbot[bot]', 'actions-bot', 'actions-user', 'ImgBotApp']

CHARSET = 'utf-8'


class Authors:
    """
    The Authors class provides methods to get the Github login of each
    commit in an efficient fashion.
    It uses the Github API and caches name+email-to-login associations
    to avoid excessive API usage.
    """

    def __init__(self, path):
        self.keys_to_user = {}
        self.path = path
        try:
            self.load()
        except FileNotFoundError:
            pass

    def set_repo(self, repo):
        self.repo = repo

    def set_github_token(self, token):
        self.token = token

    def get_login(self, key, commit_hash):
        """
        Returns the Github login associated with the given name+email key.
        A related commit hash is required in order to have a reference
        to use in the Github API request.

        Once looked up, the results are cached in a local file.
        """
        if key not in self.keys_to_user:
            if commit_hash:
                self.keys_to_user[key] = self.get_user_by_commit(commit_hash)
                self.save()
            else:
                return None
        return self.keys_to_user[key]

    def get_user_by_commit(self, hash):
        """
        Retrieves the associated Github user name for the given commit
        hash.
        """
        headers = {
            'Accept': 'application/vnd.github.v3+json',
        }
        if self.token:
            headers['Authorization'] = 'token %s' % self.token
        r = requests.get('https://api.github.com/repos/jamulussoftware/%s/commits/%s' % (self.repo, hash), headers=headers)
        if 200 <= r.status_code < 300:
            try:
                return r.json()['author']['login']
            except (TypeError, KeyError):
                logger.warning('%s has not github author, saving as empty', hash)
                return ''
        if r.status_code == 422:
            logger.warning('unable to find author of %s, saving as empty', hash)
            return ''
        raise UnexpectedGithubStatus('status was %d' % r.status_code)

    def save(self):
        """
        Saves the cache to disk.
        """
        with open(self.path, 'w') as f:
            yaml.dump(self.keys_to_user, f)

    def load(self):
        """
        Loads the cache from disk.
        """
        with open(self.path, 'r') as f:
            self.keys_to_user = yaml.safe_load(f) or {}


# Store the lookup cache right next to this script:
authors_cache = __file__.replace('.py', '-cache.yaml')

# Prepare a global Authors instance
authors = Authors(authors_cache)


def main(from_, to):
    if os.path.exists('src'):
        logger.info('scanning code repo')
        authors.set_repo('jamulus')
        print_code_contributors(from_, to)
        print_app_translators(from_, to)
    elif os.path.exists('wiki'):
        authors.set_repo('jamuluswebsite')
        print_website_contributors(from_, to)
        logger.info('scanning website repo')
    else:
        logger.error('is this a code or website repo?')


def print_code_contributors(from_, to):
    return print_contributors('Code contributors', ['.', ':!src/translation'], from_, to)


def print_app_translators(from_, to):
    return print_contributors('Application translators', ['src/translation'], from_, to)


def print_website_contributors(from_, to):
    return print_contributors('Website contributors/translators', ['.'], from_, to)


def print_contributors(title, git_log_selector, from_, to):
    contributors = ['@%s' % u for u in find_contributors(git_log_selector, from_, to) if u and u not in ignore_list]
    contributors_str = ' '.join(contributors)
    print('%s: %s' % (title, contributors_str))


def find_contributors(git_log_selector, from_, to):
    """
    Uses `git log` with the provided git_log_selector to list all commits
    starting with git reference `from_` and going until reference `to`.

    A `git_log_selector` could just be a path such as '.'.
    `from_` and `to` can be any committish such as a commit hash or a tag.
    """
    contributors = set()
    co_author_keys = set()
    commits = subprocess.check_output(['git', 'log', '-z', '--format=format:%H %an <%ae>%n%b', '%s..%s' % (from_, to), '--'] + git_log_selector)
    commits = commits.decode(CHARSET)
    for commit in commits.split('\0'):
        if not commit:
            continue
        hash, author_key = commit.split('\n', 1)[0].split(' ', 1)
        login = authors.get_login(author_key, hash)
        contributors.add(login)
        co_authors = re.findall('Co-authored-by:\s*(\S.*(<[^ >]+>))\s*\n', commit, re.I)
        for co_author_full, co_author_email in co_authors:
            login = authors.get_login(co_author_full, None)
            if not login:
                # try to find a previous commit by this mail address
                # and pass this commit id to get_login() to retrieve the
                # associated handle from the github API.
                commit = subprocess.check_output(['git', 'log', '--format=%H', '--max-count=1', '--author=%s' % re.escape(co_author_email)]).strip().decode(CHARSET)
                if commit:
                    login = authors.get_login(co_author_full, commit)
            if login:
                contributors.add(login)

    # Resolve co-authors last because we have to rely on having seen the
    # email-to-login mapping via some other commit.
    for co_author in co_author_keys:
        login = authors.get_login(co_author, None)
        contributors.add(login)
    return sorted(contributors, key=str.casefold)


if __name__ == '__main__':
    logging.basicConfig(format='%(levelname)s %(message)s')
    p = argparse.ArgumentParser(
        description='Generates a list of Github user names who contributed to a specific release.')
    p.add_argument('--from', dest='from_', required=True,
                   help='the first git hash or tag to include in the analysis, e.g. the tag of the previous release')
    p.add_argument('--to', required=True,
                   help='the last git hash or tag to include in the analysis, e.g. the (rc) tag of the target release')
    p.add_argument('--repo', required=True,
                   help='the path to the git repository to be analyzed, e.g. ./jamuluswebsite')
    p.add_argument('--github-token',
                   help='a Github Personal Access Token; optional, but might be needed if we exceed the anonymous API requests per hour limit')
    args = p.parse_args()
    os.chdir(args.repo)
    authors.set_github_token(args.github_token)
    main(args.from_, args.to)
