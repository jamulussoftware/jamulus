#!/bin/bash
set -eu

BASE_DIR=src/res/translation/wininstaller/
BASE_LANG=en
INSTALLERLNG=installerlng.nsi
BASE_LANGSTRINGS=$(grep LangString "${BASE_DIR}/${BASE_LANG}.nsi" | cut -d' ' -f2)
EXIT=0
for LANGUAGE_FILE in src/res/translation/wininstaller/{??.nsi,??_??.nsi}; do
    if [[ ${LANGUAGE_FILE} =~ /${BASE_LANG}.nsi$ ]]; then
        continue
    fi
    echo
    echo "* ${LANGUAGE_FILE}"
    echo -n "  - Checking language file is included in ${INSTALLERLNG}... "
    # shellcheck disable=SC2016  # shellcheck is confused here as NSI files use variables which look like shell variables
    # shellcheck disable=SC1003  # shellcheck misinterprets the verbatim backslash as an attempt to escape the single quote. it's not.
    if grep -q '^!include "\${ROOT_PATH}\\'"$(sed -re 's|/|\\\\|g' <<<"${LANGUAGE_FILE}")"'"' "${BASE_DIR}/${INSTALLERLNG}"; then
        echo "ok"
    else
        echo "ERROR"
        EXIT=1
    fi

    echo -n "  - Checking language file for empty strings... "
    if grep -qF '""' "${LANGUAGE_FILE}"; then
        echo "WARNING, found empty string(s)"
        EXIT=1
    else
        echo "ok"
    fi

    echo -n "  - Checking for comment leftovers from original file... "
    if head -n1 "${LANGUAGE_FILE}" | grep -qF English; then
        echo "ERROR, found 'English' in header"
        EXIT=1
    else
        echo "ok"
    fi

    echo -n "  - Checking for wrong macros... "
    LANG_MACROS=$(grep -oP '\$\{LANG_[^}]+\}' "${LANGUAGE_FILE}")
    if grep ENGLISH <<<"$LANG_MACROS"; then
        echo "ERROR, found LANG_ENGLISH"
        EXIT=1
    else
        echo "ok"
    fi

    echo -n "  - Checking for multiple lang macros... "
    if [[ $(sort -u <<<"$LANG_MACROS" | wc -l) != 1 ]]; then
        echo "ERROR, found multiple LANG_ macros"
        EXIT=1
    else
        echo "ok"
    fi

    echo -n "  - Checking if LANG_ macro is in ${INSTALLERLNG}..."
    LANG_NAME=$(sort -u <<<"${LANG_MACROS}" | sed -rne 's/\$\{LANG_(.*)\}/\1/p')
    if grep -qi '^!insertmacro MUI_LANGUAGE "'"${LANG_NAME}"'"' "$BASE_DIR/${INSTALLERLNG}"; then
        echo "ok"
    else
        echo "ERROR, not found"
        EXIT=1
    fi

    echo -n "  - Checking for LangStrings diffs... "
    OTHER_LANGSTRINGS=$(grep LangString "${LANGUAGE_FILE}" | cut -d' ' -f2)
    DIFF=$(diff -u0 <(echo "$BASE_LANGSTRINGS") <(echo "$OTHER_LANGSTRINGS") || true)
    [[ ! "${DIFF}" ]] && echo "ok" && continue
    EXIT=1
    echo "WARNING, differences found:"
    echo "${DIFF}"
    echo
done

echo
[[ $EXIT == 1 ]] && echo "PROBLEM: Errors found"
exit $EXIT
