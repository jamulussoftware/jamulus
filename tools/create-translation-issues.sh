#!/bin/bash
# This script opens translation issues for all languages via the Github API.
# The script is idempotent and can be run multiple times. It will not create
# multiple issues if milestone and title remain the same. Instead, it will
# update any existing issues with a (potentially) updated body text.
#
# Syntax:
#   ./tools/create-translation-issues.sh RELEASE DEADLINE app|web [EXTRA_TEXT]
#
# Requirements:
# - Github CLI tools + valid login (run `gh auth login` before running the script)
# - jamulus and jamulussoftware git checkouts
#
# Example usage for app translations:
#   ./tools/create-translation-issues.sh 3.8.0 2021-05-15 app 'Note: The term "Central server" has been replaced with "Directory server"'
# Example usage for web translations:
#  ../jamulus/tools/create-translation-issues.sh 3.8.0 2021-05-15 web 'Note: The term "Central server" has been replaced with "Directory server"'

set -eu

if [[ -z ${1:-} ]] || [[ -z ${2:-} ]] || [[ -z ${3:-} ]]; then
    echo "Syntax: $0 RELEASE DEADLINE app|web [EXTRA_TEXT]"
    exit 1
fi

if ! gh auth status &> /dev/null; then
    echo "Error: Please ensure that Github CLI is installed and you are logged in" > /dev/stderr
    echo "Cannot continue. Exit." > /dev/stderr
    exit 1
fi

RELEASE=$1
DEADLINE=$2
TYPE=$3
EXTRA_TEXT=${4:-}
MILESTONE="Release ${RELEASE}"
PROJECT=Tracking

# Syntax:
# TRANSLATORS_BY_LANG[TYPE_LANG]="github-handle1,github-handle2"
# with TYPE being either app or web and
# with LANG being the language code (different syntax for app and web!)
declare -A TRANSLATORS_BY_LANG
# App translators:
TRANSLATORS_BY_LANG[app_de_DE]="rolamos"
TRANSLATORS_BY_LANG[app_es_ES]="ignotus666"
TRANSLATORS_BY_LANG[app_fr_FR]="jujudusud"
TRANSLATORS_BY_LANG[app_it_IT]="dzpex"
TRANSLATORS_BY_LANG[app_nl_NL]="henkdegroot,jerogee"
TRANSLATORS_BY_LANG[app_pl_PL]="SeeLook"
TRANSLATORS_BY_LANG[app_pt_BR]="melcon"
TRANSLATORS_BY_LANG[app_pt_PT]="Snayler"
TRANSLATORS_BY_LANG[app_sk_SK]="jose1711"
TRANSLATORS_BY_LANG[app_sv_SE]="genesisproject2020"
TRANSLATORS_BY_LANG[app_zh_CN]="BLumia"
TRANSLATORS_BY_LANG[app_ko_KR]="bagjunggyu"
# Web translators:
TRANSLATORS_BY_LANG[web_de]="Helondeth,ewarning"
TRANSLATORS_BY_LANG[web_es]="ignotus666"
TRANSLATORS_BY_LANG[web_fr]="jujudusud,trebmuh"
TRANSLATORS_BY_LANG[web_it]="dzpex"
TRANSLATORS_BY_LANG[web_nl]="henkdegroot"
TRANSLATORS_BY_LANG[web_pt]="Snayler,melcon,ewarning"

# shellcheck disable=SC2016  # shellcheck can't know that this will be used with envsubst, so verbatim variables are correct here.
BODY_TEMPLATE_APP='Hi ${SPLIT_TRANSLATORS},
We are getting ready for the ${RELEASE} release. No further changes to translatable strings are currently planned for this release.

We would be happy if you updated the Jamulus software translations for **${LANG}** by **${DEADLINE}**.

Please
- Update your fork from `jamulussoftware/jamulus` `master` and create a working branch
- Update translations using Qt Linguist in your fork,
- Commit and push your changes and reference this Issue,
- Open a Pull Request before ${DEADLINE}

Suggested commit and PR message:
```
${TITLE}

Fixes #<Insert this issue'"'"'s number here>
```

${EXTRA_TEXT}${MULTIPLE_TRANSLATORS_TEXT}

Further documentation can be found in [TRANSLATING.md](https://github.com/jamulussoftware/jamulus/blob/master/docs/TRANSLATING.md).

Thanks for contributing to Jamulus!'

# shellcheck disable=SC2016  # shellcheck can't know that this will be used with envsubst, so verbatim variables are correct here.
BODY_TEMPLATE_WEB='Hi ${SPLIT_TRANSLATORS},

We are getting ready for the ${RELEASE} release and have created the [${TRANSLATE_BRANCH}](https://github.com/jamulussoftware/jamuluswebsite/tree/${TRANSLATE_BRANCH}) branch ([full diff](https://github.com/jamulussoftware/jamuluswebsite/compare/release..${TRANSLATE_BRANCH})).

We would be happy if you updated the translations for **${LANG}** by **${DEADLINE}**.

Please

- Start your work in your fork on a branch based on jamuluswebsite'"'"'s `${TRANSLATE_BRANCH}` branch.
- Update the language-specific files using your favorite editor (or directly on Github),
- New/changed images are listed at the end of this issue. [Generate new URLs](https://github.com/jamulussoftware/jamuluswebsite/tree/release#adding-screenshots) for your image `.inc` files.
- Commit and push your changes to your fork,
- Open a Pull Request with your translations to the **${TRANSLATE_BRANCH}** branch with the subject `${TITLE}`,
- Link your PR(s) to this issue by including `Fixes #<Insert this issue'"'"'s number here>` in the PR content.

${EXTRA_TEXT}${MULTIPLE_TRANSLATORS_TEXT}

Suggested commit and PR message:
```
${TITLE}

Fixes #<Insert this issue'"'"'s number here>
```

Feel free to use this Issue to discuss anything you need prior to making any PR (and in your native language if needed).

Further documentation can be found in [TRANSLATING.md](https://github.com/jamulussoftware/jamuluswebsite/blob/${TRANSLATE_BRANCH}/README.md#translating).

Thanks for contributing to Jamulus!

---

**New/Changed screenshots**:

Please [replace with new URLs](https://github.com/jamulussoftware/jamuluswebsite/tree/release#adding-screenshots) for your screenshots here, and use those URLs in your `.inc` files.

<!-- add URLs here-->

![settings-profile](https://user-images.githubusercontent.com/4561747/178144679-bef8518c-f095-4848-86bf-7639cb508505.png)

![server-window-setup](https://user-images.githubusercontent.com/4561747/178142684-1b85e654-78be-4909-a76c-945d7a0f7ccc.png)

![server-window-options](https://user-images.githubusercontent.com/4561747/178142687-da256fa5-d7b8-47ab-9265-63c3c9760841.png)

'

get_languages() {
    if [[ $TYPE == app ]]; then
        if [[ ! -f src/main.cpp ]]; then
            echo "Error: Please ensure that you are at the root of a jamulus code checkout" > /dev/stderr
            exit 1
        fi
        for LANG_FILE in src/translation/*.ts; do
            LANG=${LANG_FILE/*\/translation_/}
            LANG=${LANG/.ts/}
            echo "$LANG"
        done
    elif [[ $TYPE == web ]]; then
        if [[ ! -d wiki ]]; then
            echo "Error: Please ensure that you are at the root of a jamuluswebsite checkout" > /dev/stderr
            exit 1
        fi
        for LANG in $(cd _translator-files/po/ && ls -d -- *); do
            [[ -d _translator-files/po/$LANG ]] || continue
            [[ $LANG == en ]] && continue # does not have to be translated
            echo "$LANG"
        done
    else
        echo "Error: Invalid type. Valid types: app or website" > /dev/stderr
        exit 1
    fi
}

create_translation_issues() {
    echo "Creating ${TYPE} translation issues"

    [[ $TYPE == app ]] && GH_REPO=${GH_REPO:-jamulussoftware/jamulus} || GH_REPO=${GH_REPO:-jamulussoftware/jamuluswebsite}
    export GH_REPO # This is used by the gh tool

    for LANG in $(get_languages); do
        create_translation_issue_for_lang "$LANG"
    done
}

create_translation_issue_for_lang() {
    local lang="$1"

    translators=${TRANSLATORS_BY_LANG[${TYPE}_${lang}]-}
    if [[ -z $translators ]]; then
        echo "Warning: Can't create issue for $lang - who is responsible? Skipping." > /dev/stderr
        return
    fi

    local title="Update ${lang} ${TYPE} translation for ${RELEASE}"
    multiple_translators_text=""
    [[ $translators == *,* ]] && multiple_translators_text=$'\n\n''This Issue is assigned to multiple people. Please coordinate who will translate what part.'
    [[ $TYPE == app ]] && body_template="$BODY_TEMPLATE_APP" || body_template="$BODY_TEMPLATE_WEB"
    local body
    body=$(
        # Note: Those line continuation backslashes are required for variables
        # to be passed through:
        DEADLINE="$DEADLINE" \
            EXTRA_TEXT="$EXTRA_TEXT" \
            LANG="$lang" \
            MULTIPLE_TRANSLATORS_TEXT="$multiple_translators_text" \
            RELEASE="$RELEASE" \
            SPLIT_TRANSLATORS=$(sed -re 's/^/@/; s/,/, @/g' <<< "$translators") \
            TITLE="$title" \
            TRANSLATE_BRANCH=next-release \
            envsubst <<< "$body_template"
    )

    # Check for an existing issue
    local existing_issue
    existing_issue=$(gh issue list --milestone "$MILESTONE" --state all --search "$title" --json number --jq '.[0].number' || true)

    # If there's no existing issue, create one
    if [[ -z $existing_issue ]]; then
        echo "Creating Issue to translate $lang for $RELEASE"
        URL=$(gh issue create --title "$title" --label translation --project "$PROJECT" --body "$body" --assignee "$translators" --milestone "$MILESTONE")
        existing_issue=${URL/*\//}
    else
        echo "Issue to translate $lang for $RELEASE already exists"
    fi

    # Compare the (now) existing issue's body with the desired body and
    # update the issue if the bodies differ.
    # This is used on initial creation to fill in the issue number and it
    # can be used to update the body text afterwards.
    local online_body
    online_body=$(gh issue view "$existing_issue" --json body --jq .body)
    body=${body//<Insert this issue\'s number here>/${existing_issue}}
    if [[ "$online_body" != "$body" ]]; then
        echo "Updating Issue to translate $lang for $RELEASE"
        gh issue edit "$existing_issue" --body "$body" > /dev/null
    fi
}

create_translation_issues
