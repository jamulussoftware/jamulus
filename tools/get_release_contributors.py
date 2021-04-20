#!/usr/bin/env python3
"""
Generates a list of Github user names who contributed to a specific release.

May need a Github Personal access token to avoid hitting rate limits.

Usage:
./tools/get_release_contributors.py --from release --to translate3_7_0 --github-token YOUR_TOKEN --repo /path/to/your/jamuluswebsite
./tools/get_release_contributors.py --from r3_6_2 --to r3_7_0rc1 --github-token YOUR_TOKEN --repo /path/to/your/jamulus

"""
import requests
import argparse
import logging
import os
import subprocess
import yaml

logger = logging.getLogger('')


class UnexpectedGithubStatus(RuntimeError):
    """
    Exception which is raised when an API request fails.
    """
    pass


# List of user names which should be ignored (such as bots):
ignore_list = ['github-actions[bot]']


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
            self.keys_to_user[key] = self.get_user_by_commit(commit_hash)
            self.save()
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
            except TypeError:
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
    return print_contributors('Code contributors', ['.', ':!src/res/translation'], from_, to)


def print_app_translators(from_, to):
    return print_contributors('Application translators', ['src/res/translation'], from_, to)


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
    commits = subprocess.check_output(['git', 'log', '--format=format:%H %an <%ae>', '%s..%s' % (from_, to), '--'] + git_log_selector)
    commits = commits.decode('utf-8')
    for commit in commits.split('\n'):
        hash, author_key = commit.split(' ', 1)
        login = authors.get_login(author_key, hash)
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
