#!/usr/bin/env python3
"""
Generates a list of GitHub usernames who contributed to a specific release.

May need a GitHub Personal access token to avoid hitting rate limits.

Usage:

## Website repo (jamulussoftware/jamuluswebsite)
- cd jamuluswebsite
# Find commit ID of previous version. Search for "RELEASE:"
- git log --oneline release
- /path/to/jamulussoftware/code/tools/get_release_contributors.py \
    --from COMMIT_ID_FROM_PREVIOUS_STEP --to next-release \
    --github-token YOUR_TOKEN --repo .

## Code repo (jamulussoftware/jamulus)
- git pull -C /path/
- ./tools/get_release_contributors.py --from r3_8_1 --to r3_8_2 \
    --github-token YOUR_TOKEN --repo .

"""
import argparse
import logging
import os
import re
import subprocess

import requests
import yaml

logger = logging.getLogger('')


class UnexpectedGithubStatus(RuntimeError):
    """
    Exception which is raised when an API request fails.
    """


# List of contributor names which should be ignored (such as bots):
ignore_list = ['@github-actions[bot]', '@imgbot[bot]', '@actions-bot',
               '@actions-user', '@ImgBotApp', '@dependabot[bot]', '@weblate']

CHARSET = 'utf-8'


class Authors:
    """
    The Authors class provides methods to get the GitHub login of each
    commit in an efficient fashion.
    It uses the GitHub API and caches name+email-to-login associations
    to avoid excessive API usage.
    """

    def __init__(self, path):
        self.keys_to_user = {}
        self.path = path
        self.repo = None
        self.token = None
        try:
            self.load()
        except FileNotFoundError:
            pass

    def set_repo(self, repo):
        self.repo = repo

    def set_github_token(self, token):
        self.token = token

    def _get_login(self, key, commit_hash):
        """
        Returns the GitHub login associated with the given name+email key.
        A related commit hash is required in order to have a reference
        to use in the GitHub API request.

        Once looked up, the results are cached in a local file.
        """
        if key not in self.keys_to_user:
            user = self.get_user_by_email(key)
            if not user and commit_hash:
                user = self.get_user_by_commit(commit_hash)

            if user:
                self.keys_to_user[key] = user
                self.save()

        return self.keys_to_user.get(key, None)

    def get_login_or_realname(self, key, commit_hash):
        """
        Returns the Github login (@-prefixed) or, if not found, the real name from
        the given name+email key.
        """
        login = self._get_login(key, commit_hash)
        if login:
            return f'@{login}'
        matches = re.match(r'\A([^@<>]+) <.*>\Z', key)
        if matches:
            return matches.group(1)
        logger.warning("unable to extract GitHub login or real name from %r", key)
        return None

    def get_user_by_commit(self, sha):
        """
        Retrieves the associated GitHub username for the given commit hash.
        """
        resp = self._github_api_get(f'repos/jamulussoftware/{self.repo}/commits/{sha}')
        if 200 <= resp.status_code < 300:
            try:
                return resp.json()['author']['login']
            except (TypeError, KeyError):
                logger.warning('%s has not GitHub author, saving as empty', sha)
                return ''
        if resp.status_code == 422:
            logger.warning('unable to find author of %s, saving as empty', sha)
            return ''
        raise UnexpectedGithubStatus(f"status was {resp.status_code}")

    def _github_api_get(self, path, *args, **kwargs):
        headers = {
            'Accept': 'application/vnd.github.v3+json',
        }
        if self.token:
            headers['Authorization'] = f"token {self.token}"
        return requests.get(f'https://api.github.com/{path}', *args, headers=headers, **kwargs,
                            timeout=10)

    def get_user_by_email(self, key):
        m = re.match(r'\A[^<]+<([^<> ]+@[^<> ]+)>\Z', key)
        if not m:
            return None
        email = m.group(1)
        # Handle Github-generated email addresses via static matching:
        m = re.match(r'\A(\d+\+)?([^+@]+)@users\.noreply\.github\.com\Z', email)
        if m:
            return m.group(2)
        resp = self._github_api_get('search/users', params={'q': f'{email} in:email'})
        if resp.status_code < 200 or resp.status_code >= 300:
            logger.warning('search/users for %s failed with code %s', email, resp.status_code)
            return None
        items = resp.json().get('items', [])
        for item in items:
            login = item['login']
            u = self._github_api_get(f'users/{login}').json()
            if u.get('email', '') == email:
                return login

        logger.warning('unable to find a github profile with public email %s', email)
        return None

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
    contributors = [u for u in find_contributors(git_log_selector, from_, to) if
                    u and u not in ignore_list]
    contributors_str = ', '.join(contributors)
    print(f'{title}: {contributors_str}')


def find_contributors(git_log_selector, from_ref, to_ref):
    """
    Uses `git log` with the provided git_log_selector to list all commits
    starting with git reference `from_ref` and going until reference `to_ref`.

    A `git_log_selector` could just be a path such as '.'.
    `from_` and `to` can be any committish such as a commit hash or a tag.
    """
    contributors = set()
    co_author_keys = set()
    commits = subprocess.check_output(
        ['git', 'log', '-z', '--show-pulls', '--format=format:%H %an <%ae>%n%b',
         f'{from_ref}..{to_ref}', '--'] + git_log_selector)
    commits = commits.decode(CHARSET)
    for commit in commits.split('\0'):
        if not commit:
            continue
        sha, author_key = commit.split('\n', 1)[0].split(' ', 1)
        contributor = authors.get_login_or_realname(author_key, sha)
        contributors.add(contributor)
        co_authors = re.findall(r'Co-authored-by:\s*(\S.*(<[^ >]+>))\s*(?:$|\n)', commit, re.I)
        for co_author_full, co_author_email in co_authors:
            logger.debug('checking co author %s', co_author_full)
            contributor = authors.get_login_or_realname(co_author_full, None)
            if not contributor or not contributor.startswith('@'):
                # try to find a previous commit by this mail address
                # and pass this commit id to get_login_or_realname() to retrieve the
                # associated handle from the GitHub API.
                esc_email = re.escape(co_author_email)
                commit = subprocess.check_output(['git', 'log', '--format=%H', '--max-count=1',
                                                  f'--author={esc_email}']).strip().decode(CHARSET)
                if commit:
                    contributor = authors.get_login_or_realname(co_author_full, commit)
            if contributor:
                contributors.add(contributor)

    # Resolve co-authors last because we have to rely on having seen the
    # email-to-login mapping via some other commit.
    for co_author in co_author_keys:
        contributor = authors.get_login_or_realname(co_author, None)
        if contributor:
            contributors.add(contributor)
        else:
            # TODO: check this is the right logic...
            contributors.add(co_author)
    return sorted(contributors, key=str.casefold)


if __name__ == '__main__':
    p = argparse.ArgumentParser(
        description='Generates a list of Github user names who contributed to a specific release.')
    p.add_argument('--from', dest='from_', required=True,
                   help='the first git hash or tag to include in the analysis, '
                        'e.g. the tag of the previous release')
    p.add_argument('--to', required=True,
                   help='the last git hash or tag to include in the analysis, '
                        'e.g. the (rc) tag of the target release')
    p.add_argument('--repo', required=True,
                   help='the path to the git repository to be analyzed, e.g. ./jamuluswebsite')
    p.add_argument('--github-token',
                   help='a Github Personal Access Token; optional, but might be needed '
                        'if we exceed the anonymous API requests per hour limit')
    p.add_argument('--verbose', '-v', action='store_true',
                   help='enable verbose output')
    p.add_argument('--quiet', '-q', action='store_true',
                   help='only log errors')
    args = p.parse_args()
    if args.verbose and args.quiet:
        p.error('--verbose and --quiet are mutually exclusive')
    if args.verbose:
        level = logging.DEBUG
    elif args.quiet:
        level = logging.ERROR
    else:
        level = logging.WARNING
    logging.basicConfig(format='%(levelname)s %(message)s', level=level)
    os.chdir(args.repo)
    authors.set_github_token(args.github_token)
    main(args.from_, args.to)
