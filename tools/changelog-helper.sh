#!/bin/bash
# Requirements: git, Github CLI (gh), jq
set -eu

echo "This tool checks the ChangeLog file and compares its entries for the top-most"
echo "release against the associated Github milestone and the git log."
echo "It will mention any PRs which are not listed in the ChangeLog."
echo "It can optionally pre-fill the ChangeLog with all missing entries"
echo "or sort existing entries. See --help."
echo

# Ensure that we list upstream PRs and not those from the fork:
export GH_REPO=jamulussoftware/jamulus

PR_LIST_LIMIT=300
TRANSLATION_ENTRY_TEXT="GUI: Translations have been updated:"
declare -A LANGS
LANGS[de_DE]="German"
LANGS[fr_FR]="French"
LANGS[it_IT]="Italian"
LANGS[nl_NL]="Dutch"
LANGS[pl_PL]="Polish"
LANGS[pt_BR]="Portuguese Brazilian"
LANGS[pt_PT]="Portuguese European"
LANGS[sk_SK]="Slovak"
LANGS[es_ES]="Spanish"
LANGS[sv_SE]="Swedish"
LANGS[zh_CN]="Simplified Chinese"

find_or_add_missing_entries() {
    local changelog
    changelog=$(sed -rne '/^###.*'"${target_release//./\.}"'\b/,/^### '"${prev_release//./\.}"'\b/p' ChangeLog)
    local changelog_begin_position
    changelog_begin_position=$(grep -nP '^### .*\d+\.\d+\.\d+\b' ChangeLog | head -n1 | cut -d: -f1)

    echo "Checking if all merged Github PRs since ${prev_release} are included for ${target_release}..."
    for id in $(gh pr list --limit "${PR_LIST_LIMIT}" --search 'milestone:"Release '"${target_release}"'"' --state merged | awk '{print $1}'); do
        check_or_add_pr "$id"
    done

    local target_ref=origin/master
    if git tag | grep -qxF "${target_release_tag}"; then
        # already released, use this
        target_ref="${target_release_tag}"
    fi
    echo
    echo "Checking if all PR references in git log since ${prev_release_tag} are included for ${target_release} based on ref ${target_ref}..."
    local milestone
    for id in $(git log "${prev_release_tag}..master" | grep -oP '#\K(\d+)'); do
        gh pr view "${id}" --json title &> /dev/null || continue # Skip non-PRs
        milestone=$(gh pr view "${id}" --json milestone --jq .milestone.title)
        if [[ "${milestone}" =~ "Release " ]] && [[ "${milestone}" != "Release ${target_release}" ]]; then
            echo "-> Ignoring PR #${id}, which was mentioned in 'git log ${prev_release_tag}..HEAD', but already has milestone '${milestone}'"
            continue
        fi
        check_or_add_pr "${id}"
    done

    echo
    echo "Done."
    if [[ "${ACTION}" == "find-missing-entries" ]]; then
        echo "You can re-run this script with add-missing-entries to fill the ChangeLog automatically."
    fi
}

group_entries() {
    local changelog_begin_position
    changelog_begin_position=$(grep -nP '^### .*\d+\.\d+\.\d+\b' ChangeLog | head -n1 | cut -d: -f1)
    local changelog_prev_release_position
    changelog_prev_release_position=$(grep -nP '^### .*\d+\.\d+\.\d+\b' ChangeLog | head -n2 | tail -n1 | cut -d: -f1)

    # Save everything before the actual release changelog content:
    local changelog_header
    changelog_header=$(head -n "${changelog_begin_position}" ChangeLog)

    # Save everything after the actual release changelog content:
    local changelog_prev_releases
    changelog_prev_releases=$(tail -n "+${changelog_prev_release_position}" ChangeLog)

    # Save the current release's changelog content:
    local changelog
    changelog=$(sed -rne '/^###.*'"${target_release//./\.}"'\b/,/^### '"${prev_release//./\.}"'\b/p' ChangeLog | tail -n +2 | head -n -1)

    # Remove trailing whitespace on all lines of the current changelog:
    changelog=$(sed -re 's/\s+$//' <<< "$changelog")

    # Prepend a number to known categories in order to make their sorting position consistent:
    category_order=(
        "$TRANSLATION_ENTRY_TEXT"
        "GUI"
        "Accessibility"
        "Client"
        "Server"
        "Recorder"
        "Performance"
        "CLI"
        "Bug Fix"
        "Windows"
        "Installer"
        "Linux"
        "Mac"
        "Android"
        "iOS"
        "Translation"
        "Doc"
        "Website"
        "Github"
        "Build"
        "Autobuild"
        "Code"
        "Internal"
    )
    local index=0
    for category in "${category_order[@]}"; do
        changelog=$(sed -re 's/^(- '"${category}"')/'"${index}"' \1/' <<< "${changelog}")
        index=$((index + 1))
    done

    # Reduce blocks ("entries") to a single line by replacing \n with \v.
    # `sort` then works on those reduced lines and sorts them by the category (e.g. Server:)
    # Afterwards, convert \v to \n again:
    changelog=$(
        sed -r ':r;/(^|\n)$/!{$!{N;br}};s/\n/\v/g' <<< "$changelog" |
            LC_ALL=C sort --stable --numeric-sort --field-separator=':' -k1,1 |
            sed 's/\v/\n/g'
    )

    # Remove temporary sorting indices at line start again:
    changelog=$(sed -re 's/^[0-9]+ (- )/\1/' <<< "$changelog")

    # Rebuild the changelog and write back to file:
    (echo "$changelog_header" && echo "$changelog" && echo && echo && echo "$changelog_prev_releases") > ChangeLog
}

declare -A checked_ids=()
check_or_add_pr() {
    local id=$1
    if [[ "${checked_ids[$id]+exists}" ]]; then
        return
    fi
    checked_ids[$id]=1
    if grep -qE "#$id\>" <<< "$changelog"; then
        # Changelog already lists this PR ID -> nothing to do
        # (\> ensures that we only match full, standalone IDs)
        return
    fi
    local json
    json=$(gh pr view "${id/#/}" --json title,author,state)
    local state
    state=$(jq -r .state <<< "${json}")
    local title
    title=$(jq -r .title <<< "${json}" | sanitize_title)
    local author
    author=$(jq -r .author.login <<< "${json}")
    if [[ "${state}" != "MERGED" ]]; then
        echo "-> Ignoring PR #${id} as state ${state} != MERGED"
        return
    fi
    local title_suggestion_in_pr
    title_suggestion_in_pr=$(gh pr view "$id" --json body,comments,reviews --jq '(.body), (.comments[] .body), (.reviews[] .body)' |
        grep -oP '\bCHANGELOG:\s*\K([^\\]{5,})' | tail -n1 | sanitize_title)
    if [[ "${title_suggestion_in_pr}" ]]; then
        title="${title_suggestion_in_pr}"
        if [[ "${title_suggestion_in_pr}" == "SKIP" ]]; then
            return
        fi
    fi
    echo -n "-> Missing PR #${id} (${title}, @${author})"
    if [[ "${ACTION}" != add-missing-entries ]]; then
        echo
        return
    fi
    echo ", adding new entry"
    local new_entry=""
    local lang
    lang=$(grep -oP 'Updated? \K(\S+)(?= app translations? for )' <<< "$title" || true)
    if [[ "${lang}" ]]; then
        # Note: This creates a top-level entry for each language.
        # group-entries can merge those to a single one.
        local full_lang="${LANGS[$lang]-${lang}}"
        add_translation_pr "${lang}" "${author}" "${id}"
        return
    fi
    new_entry=$(
        echo "- ${title} (#${id})."
        echo "  (contributed by @${author})"
    )
    local changelog_before
    changelog_before=$(head -n "${changelog_begin_position}" ChangeLog)
    local changelog_after
    changelog_after=$(tail -n "+$((changelog_begin_position + 1))" ChangeLog)
    (echo "$changelog_before" && echo && echo "$new_entry" && echo "$changelog_after") > ChangeLog
}

add_translation_pr() {
    local lang="${1}"
    local author="${2}"
    local id="${3}"
    local changelog_begin_position
    changelog_begin_position=$(grep -nP '^### .*\d+\.\d+\.\d+\b' ChangeLog | head -n1 | cut -d: -f1)
    local changelog_prev_release_position
    changelog_prev_release_position=$(grep -nP '^### .*\d+\.\d+\.\d+\b' ChangeLog | head -n2 | tail -n1 | cut -d: -f1)

    # Save everything before the actual release changelog content:
    local changelog_header
    changelog_header=$(head -n "${changelog_begin_position}" ChangeLog)

    # Save everything after the actual release changelog content:
    local changelog_prev_releases
    changelog_prev_releases=$(tail -n "+${changelog_prev_release_position}" ChangeLog)

    # Save the current release's changelog content:
    local changelog
    changelog=$(sed -rne '/^###.*'"${target_release//./\.}"'\b/,/^### '"${prev_release//./\.}"'\b/p' ChangeLog | tail -n +2 | head -n -1)
    local changelog_orig="${changelog}"

    # Is there an existing entry for this language already?
    changelog=$(sed -re "s/^(  \* ${full_lang}, by .+ \(.*)\)/\1, #${id})/" <<< "${changelog}")
    if [[ "${changelog}" == "${changelog_orig}" ]]; then
        # No existing language entry. Check for an existing translation entry.
        changelog=$(sed -re "s/^(- ${TRANSLATION_ENTRY_TEXT}.*)/\1\n  * ${full_lang}, by @${author} (#${id})/" <<< "${changelog}")
        if [[ "${changelog}" == "${changelog_orig}" ]]; then
            # No existing translation entry at all. Add a new one.
            changelog="${changelog}$(
                echo
                echo
                echo "- ${TRANSLATION_ENTRY_TEXT}"
                echo "  * ${full_lang}, by @${author} (#${id})"
            )"
        else
            # Existing translation entries, so sort them:
            local changelog_before_translations=""
            local changelog_translations=""
            local changelog_after_translations=""
            local changelog_translations_pos=before # before|block|after
            while IFS= read -r line; do
                if [[ "${changelog_translations_pos}" == "before" ]]; then
                    if [[ "${line}" == "- ${TRANSLATION_ENTRY_TEXT}" ]]; then
                        changelog_translations_pos=block
                    fi
                    changelog_before_translations="${changelog_before_translations}${line}"$'\n'
                    continue
                fi
                if [[ "${changelog_translations_pos}" == "block" ]]; then
                    if [[ "${line}" == "" ]]; then
                        changelog_translations_pos=after
                        # fallthrough
                    else
                        changelog_translations="${changelog_translations}${line}"$'\n'
                        continue
                    fi
                fi
                if [[ "${changelog_translations_pos}" == "after" ]]; then
                    changelog_after_translations="${changelog_after_translations}${line}"$'\n'
                fi
            done <<< "${changelog}"
            changelog=$(
                # echo -n strips whitespace. we need that here.
                echo -n "${changelog_before_translations}"
                echo -n "$(grep -vP '^$' <<< "${changelog_translations}" | sort)"
                echo -n "${changelog_after_translations}"
            )
        fi
    fi
    # Rebuild the changelog and write back to file:
    (echo "$changelog_header" && echo "$changelog" && echo && echo "$changelog_prev_releases") > ChangeLog
}

sanitize_title() {
    sed \
        -re 's/^\s+//' \
        -re 's/\s{2,}/ /' \
        -re 's/\s*\.?\s*$//' \
        -re 's/\b((Add)|(Updat|Enhanc|Improv|Remov)e)\b/\2\3ed/i'
}

case "${1:-1}" in
    find-missing-entries)
        ACTION=find-missing-entries
        ;;
    add-missing-entries)
        ACTION=add-missing-entries
        ;;
    group-entries)
        ACTION=group-entries
        ;;
    --help)
        echo "Usage: $0 ACTION"
        echo "  Supported actions:"
        echo "    * find-missing-entries: Prints a list"
        echo "    * add-missing-entries: Inserts missing entries into the file"
        echo "    * group-entries: Groups existing entries by prefix"
        echo
        exit
        ;;
    *)
        echo "ERROR: Bad invocation, see --help"
        exit 1
        ;;
esac

target_release=$(grep -oP '^### .*\K(\d+\.\d+\.\d+)\b' ChangeLog  | head -n1)
prev_release=$(grep -oP '^### .*\K(\d+\.\d+\.\d+)\b' ChangeLog  | head -n2 | tail -n1)
target_release_tag=r${target_release//./_}
prev_release_tag=r${prev_release//./_}

echo "Auto-detected target release: ${target_release}"
echo "Auto-detected previous release: ${prev_release}"
echo

case "$ACTION" in
    find-missing-entries | add-missing-entries)
        find_or_add_missing_entries
        ;;
    group-entries)
        group_entries
        ;;
esac
