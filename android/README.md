# Android deployment

<!-- Copyright (c) 2026 The Jamulus Development Team.
     Licensed under AGPL 3.0 or any later version. See ../COPYING for details. -->

Android builds are experimental. For toolchains and local build commands, see
[COMPILING.md](../COMPILING.md#android). This guide covers the identities and
access needed to distribute builds from a GitHub repository. Do not commit
keystores, passwords, or generated Google credentials.

## What is distributed?

The normal GitHub Auto-Build produces a Qt 5 multi-ABI debug APK and, when
signing is configured, a release APK. The experimental Qt 6 build produces
ZIPs containing one APK per ABI. Download the desired run artifact and, for
Qt 6, extract the APK for the device's ABI.
These APKs can be installed directly, for example with `adb install path/to.apk`.

The separate, manually requested Play build produces a Qt 5 release Android App
Bundle (AAB). GitHub uploads that bundle to the **internal testing track** as a
completed release, not to production. When requested, the workflow builds only
the AAB, not the normal APKs or other targets. The local `play-store` stage
only copies the bundle into `play-store/`; it does not upload anything.

For a local Play build, optionally copy
`tools/android-build_qt5-play-store.settings.example` to
`android-build_qt5-play-store.settings` and set the toolchain and release
signing values. This profile uses a separate API 36 toolchain from the regular
Qt 5 APK build. Then run:

```bash
tools/android-build.sh --stream qt5 build get-artifacts play-store
```

This writes `deploy/jamulus_<version>_android_release_qt5.aab` and copies it to
`play-store/`. The release keystore must match the Play app's registered upload
certificate; AAB builds do not fall back to a debug key. Upload the resulting
AAB manually in Play Console to create a testing-track release, if not using
the GitHub workflow.

**Internal testing** and **Internal App Sharing** are different Play features.
The current workflow does not upload to Internal App Sharing or produce its
download link. See [Testing with Play](#testing-with-play) for both paths.

## Application identity and signing

An Android application ID identifies the app; the signing certificate
authenticates its updates. Android generally accepts an update only when its
application ID and signing identity match the installed app. Keep the release
signing key stable and backed up securely: losing it can prevent updates to
side-loaded installations.

The build scripts require a signing key for release APKs so that subsequent
releases can update existing installations and recipients can verify a
consistent publisher identity. Debug builds use Gradle's normal debug signing
regardless of release credentials. A development keystore can sign local
release-variant APKs, but its well-known credentials make it unsuitable for
publication.

For Play, distinguish the **upload key**, which signs the submitted AAB, from
the **app signing key**, which Play uses to sign APKs delivered to devices.
They may be different. The current workflow uses the same configured keystore
for side-load release APKs and Play AAB uploads. That does **not** make a
side-loaded APK update-compatible with a Play-installed app if Play uses a
different app signing certificate. Plan the keys and compare certificate
fingerprints before promising cross-channel updates; Google's
[app signing guide](https://developer.android.com/studio/publish/app-signing)
describes the options and upload-key reset process. Internal App Sharing uses
yet another, Google-managed test certificate.

Choose the application ID deliberately before the first Play upload: Play
fixes it for that app, and changing it creates a different app. The workflow
defaults to `app.<owner>.<repo>` (lower-cased, invalid characters replaced by
`_`), so the official repository builds and uploads as
`app.jamulussoftware.jamulus`. Set the GitHub repository variable
`ANDROID_PACKAGE_NAME` to override it. Setting `JAMULUS_ANDROID_PACKAGE_NAME`
locally overrides the default. The checked-in templates contain the placeholder
`++JAMULUS_ANDROID_PACKAGE_NAME++`, which the scripts replace in a temporary copy
(a build fails if no ID is set). The scripts apply the ID to the generated Android manifest and Play Gradle namespace without
editing the checked-in templates; the workflow uses the same value for the Play
API. Debug APKs append `.debug` to the ID so they can be installed alongside a
release build.

## GitHub settings

In the repository, open **Settings > Secrets and variables > Actions** and add
the following repository secrets. Use the existing key for an established app;
creating a new key does not reproduce an existing signing identity.

| Secret | Meaning |
| --- | --- |
| `ANDROID_KEYSTORE` | Base64 encoding of the binary release keystore, not a path or a certificate. |
| `ANDROID_KEY_ALIAS` | Alias of the private signing key in that keystore. |
| `ANDROID_KEYSTORE_PASSWORD` | Password for the keystore. |
| `ANDROID_KEY_PASSWORD` | Password for the private key. |

All four are required by the GitHub workflow for release signing. If any is
missing, normal Android builds produce only debug artifacts and report that
the release was skipped; a requested Play upload fails early. With all four,
normal builds produce both debug and release artifacts. Verify the signing
result before distribution.

### Create a new signing identity

For a new app or signing identity, the following Linux example creates a
private directory outside the repository and a PKCS12 keystore containing an
RSA signing key and certificate. Use a new directory that does not already
exist. `keytool` prompts for a keystore password and certificate details;
choose a strong password. The validity is 10,000 days. See also the
[Android key-generation instructions](https://developer.android.com/studio/publish/app-signing#generate-key).

```bash
mkdir -m 700 "$HOME/android-signing"
keytool -genkeypair \
  -keystore "$HOME/android-signing/upload.p12" \
  -storetype PKCS12 -alias release_upload \
  -keyalg RSA -keysize 4096 -validity 10000
```

`release_upload` is the example key alias; choose another if appropriate and
use it consistently below. Avoid secret values that occur in the application
ID or other workflow outputs: GitHub may mask them and skip affected job
outputs. For the PKCS12 keystore created by this command, use the same password
for the key and keystore. Record the alias and SHA-256 certificate fingerprint
and back up the keystore and password separately, with restricted access.
This command displays the public certificate fingerprint and prompts for the
keystore password:

```bash
keytool -list -v -keystore "$HOME/android-signing/upload.p12" -alias release_upload
```

To configure GitHub Actions, replace `OWNER/REPO` with the target repository
in the following commands. The first encodes the binary keystore and sends it
to GitHub without printing it; the second stores its alias:

```bash
base64 -w 0 "$HOME/android-signing/upload.p12" | gh secret set ANDROID_KEYSTORE --repo OWNER/REPO
printf '%s' release_upload | gh secret set ANDROID_KEY_ALIAS --repo OWNER/REPO
```

Set the keystore and key passwords at the interactive prompts, using the same
password for both with the example keystore. Alternatively, use the GitHub UI.
Do not put passwords on the command line.

```bash
gh secret set ANDROID_KEYSTORE_PASSWORD --repo OWNER/REPO
gh secret set ANDROID_KEY_PASSWORD --repo OWNER/REPO
```

### Repository variables

Set these repository **variables**:

| Variable | Meaning |
| --- | --- |
| `ANDROID_PACKAGE_NAME` | Application ID override for APKs and the Play bundle. Defaults to `app.<owner>.<repo>` if unset (the official repository uses `app.jamulussoftware.jamulus`). |
| `GCP_WORKLOAD_IDENTITY_PROVIDER` | Full provider resource name: `projects/PROJECT_NUMBER/locations/global/workloadIdentityPools/POOL_ID/providers/PROVIDER_ID`. Required for Play. |
| `GCP_SERVICE_ACCOUNT_EMAIL` | Email of the Google Cloud service account granted access to the Play app. Required for Play. |

`GITHUB_TOKEN` is supplied by GitHub, not configured as a repository secret.
The Play job also requests `id-token: write` for GitHub OIDC. Repository
variables and secrets must be available to the selected workflow run; fork
pull requests and repository or organization policies can restrict them.
Protect the branches and users allowed to run workflows that can access release
signing credentials or request a Play upload.

### Local equivalents and build controls

For local builds, put the signing values in an ignored settings file or export
them in the environment. The wrapper sources a settings file as shell code, so
only use a file you trust. It exports settings-file values to the build driver:

| Local variable | Purpose |
| --- | --- |
| `JAMULUS_ANDROID_KEYSTORE` | Path to a readable release keystore; alternatively use the base64 variable below. |
| `JAMULUS_ANDROID_KEYSTORE_BASE64` | Base64-encoded keystore; mutually exclusive with the file path. |
| `JAMULUS_ANDROID_KEY_ALIAS` | Alias in the release keystore. |
| `JAMULUS_ANDROID_KEYSTORE_PASSWORD` | Release keystore password. |
| `JAMULUS_ANDROID_KEY_PASSWORD` | Release key password; pass it when the key requires one. The GitHub workflow requires it. |
| `JAMULUS_ANDROID_DEV_KEYSTORE` | Path to a reusable development-only keystore for release APKs. If missing, it is created and retained with Android debug-key credentials (`androiddebugkey` / `android`). An existing file must be a valid Android debug keystore. Do not combine with release signing credentials or use for publishing. |
| `JAMULUS_ANDROID_PACKAGE_NAME` | Optional application ID override; use the same ID as the intended Play app for Play builds. |
| `JAMULUS_ANDROID_VERSION_CODE` | Optional explicit Android version code. Use a valid positive code appropriate to the target app. |
| `JAMULUS_BUILD_VERSION` | Build/artifact version; the local wrapper otherwise derives it from `Jamulus.pro` and Git. |
| `JAMULUS_ANDROID_STREAM` | `qt5` or `qt6`; the wrapper's `--stream` selects it. |
| `JAMULUS_ANDROID_PLAY_STORE_BUILD` | Selects the Play profile when `true`; the wrapper sets it when the `play-store` stage is requested. |
| `JAMULUS_ANDROID_PACKAGE_FORMAT` | `apk` normally, `aab` for the Play profile; AAB requires Qt 5, Play mode and release-only builds. |
| `BUILD_MODES`, `TARGET_ARCHS` | Build variants and ABI list; the wrapper also accepts `--build-modes` and `--archs`. |
| `JAMULUS_ANDROID_QMAKE_CONFIG` | Optional space-separated qmake `CONFIG` values to add to each build. |
| `JAMULUS_ANDROID_ARTIFACT_SUFFIX` | Optional artifact filename suffix; keep the default for the Play bundle selector. |
| `JAMULUS_ANDROID_BUILD_DIR`, `JAMULUS_ANDROID_DEPLOY_DIR` | Optional build and artifact output directories. |
| `JAMULUS_ANDROID_SETTINGS` | Optional settings-file path when `--settings` is not supplied. |

For example, add this to the selected settings file or export it in the
environment (a settings-file value overrides the environment):

```bash
JAMULUS_ANDROID_QMAKE_CONFIG="disable_version_check nojsonrpc"
```

Each value is passed to qmake as `CONFIG+=<value>`. See the
[compile time arguments](../COMPILING.md#compile-time-arguments) for options.
For GitHub builds, set `JAMULUS_ANDROID_QMAKE_CONFIG` in the build step's
`env` in [autobuild.yml](../.github/workflows/autobuild.yml); a repository
variable alone is not passed to the build.

Toolchain paths and version overrides are described in
[COMPILING.md](../COMPILING.md#android) and the
[settings examples](../tools/). Avoid placing keystores inside generated
output directories. `--log` captures build output; treat logs as potentially
sensitive, especially when using verbose tools.

Local APK builds default to debug mode. Request a release with
`--build-modes release` or `--build-modes "debug release"`, or set
`BUILD_MODES` explicitly in a settings file or the environment. The Play
profile defaults to release. When using a development key, choose an
application ID different from official releases. A debug APK and a
development-release APK with the same ID can update each other only if their
signing keys match.

For Play uploads the workflow sets `JAMULUS_ANDROID_VERSION_CODE` to
`GITHUB_RUN_NUMBER * 1000 + GITHUB_RUN_ATTEMPT`, subject to its range checks.
Run numbers are scoped to a workflow in a repository: they do not ensure
monotonicity across repositories, other workflows, or manual uploads. Check the
existing Play version codes before moving publication to a new repository.
`JAMULUS_BUILD_VERSION` supplies the artifact and Play release name; it is not
the Android version code.

## Google Cloud setup

The workflow exchanges a GitHub OIDC token for short-lived credentials using
Workload Identity Federation (WIF), then impersonates a service account. It
does not need a downloaded service-account JSON key. An administrator needs
permission to enable APIs, create a service account, administer WIF pools and
providers, and change that service account's IAM policy. Google recommends a
dedicated project for pools and requires billing enabled; see the
[Google WIF guide](https://cloud.google.com/iam/docs/workload-identity-federation-with-deployment-pipelines)
for prerequisites and propagation delays.

The examples below restrict access to one repository and the `main` branch.
Use the actual protected publication branch if different. Obtain numeric
repository and owner IDs, for example with
`gh api repos/OWNER/REPO --jq '[.id,.owner.id] | @tsv'`. Numeric IDs avoid
trusting a repository name that could later be reused. Restrict who may edit
the trusted branch and workflow: a workflow on that branch can request the
service account's identity.

### Cloud Console

1. Select or create a Cloud project and enable billing. Under **APIs &
   Services > Library**, enable IAM, Cloud Resource Manager, IAM Service
   Account Credentials, Security Token Service and Google Play Developer API.
2. Under **IAM & Admin > Service Accounts**, create a dedicated service
   account. Record its email; no service-account key is needed.
3. Under **IAM & Admin > Workload Identity Federation**, create a pool and an
   OIDC provider. Set issuer to `https://token.actions.githubusercontent.com/`
   and use the default audience. Add these attribute mappings:

   ```text
   google.subject=assertion.sub
   attribute.repository_id=assertion.repository_id
   attribute.repository_owner_id=assertion.repository_owner_id
   attribute.ref=assertion.ref
   ```

4. Set the provider condition below, substituting the numeric IDs and
   publication branch:

   ```text
   attribute.repository_id=='REPOSITORY_ID' && attribute.repository_owner_id=='OWNER_ID' && attribute.ref=='refs/heads/main'
   ```

5. In the pool's **Grant access** flow, grant service-account impersonation
   to identities matching `repository_id` and the numeric repository ID.
   The binding must grant **Workload Identity User**
   (`roles/iam.workloadIdentityUser`) on the service account. The provider
   condition further limits the owner and branch.
6. Copy the full provider resource name, using the **project number**, and
   the service-account email into the GitHub variables above. Give the
   service account Play permissions as described below; Cloud IAM permissions
   alone do not grant access to Play Console.

### Equivalent `gcloud` commands

Run these as an authorized administrator in Cloud Shell, or run
`gcloud auth login` first in a local shell. Replace all placeholder values.
These commands create new resources; reuse existing resources rather than
recreating them if appropriate.

```bash
PROJECT_ID=YOUR_CLOUD_PROJECT_ID
REPOSITORY_ID=YOUR_NUMERIC_GITHUB_REPOSITORY_ID
OWNER_ID=YOUR_NUMERIC_GITHUB_OWNER_ID
POOL_ID=github-actions
PROVIDER_ID=github
SERVICE_ACCOUNT_ID=jamulus-play
BRANCH=main

gcloud config set project "$PROJECT_ID"
gcloud services enable iam.googleapis.com cloudresourcemanager.googleapis.com \
  iamcredentials.googleapis.com sts.googleapis.com androidpublisher.googleapis.com
PROJECT_NUMBER="$(gcloud projects describe "$PROJECT_ID" --format='value(projectNumber)')"
SERVICE_ACCOUNT_EMAIL="${SERVICE_ACCOUNT_ID}@${PROJECT_ID}.iam.gserviceaccount.com"
gcloud iam service-accounts create "$SERVICE_ACCOUNT_ID" --display-name="Jamulus Play upload"
gcloud iam workload-identity-pools create "$POOL_ID" --location=global \
  --display-name="GitHub Actions"
gcloud iam workload-identity-pools providers create-oidc "$PROVIDER_ID" \
  --location=global --workload-identity-pool="$POOL_ID" \
  --issuer-uri="https://token.actions.githubusercontent.com/" \
  --attribute-mapping="google.subject=assertion.sub,attribute.repository_id=assertion.repository_id,attribute.repository_owner_id=assertion.repository_owner_id,attribute.ref=assertion.ref" \
  --attribute-condition="attribute.repository_id=='${REPOSITORY_ID}' && attribute.repository_owner_id=='${OWNER_ID}' && attribute.ref=='refs/heads/${BRANCH}'"
gcloud iam service-accounts add-iam-policy-binding "$SERVICE_ACCOUNT_EMAIL" \
  --role=roles/iam.workloadIdentityUser \
  --member="principalSet://iam.googleapis.com/projects/${PROJECT_NUMBER}/locations/global/workloadIdentityPools/${POOL_ID}/attribute.repository_id/${REPOSITORY_ID}"
printf 'GCP_WORKLOAD_IDENTITY_PROVIDER=projects/%s/locations/global/workloadIdentityPools/%s/providers/%s\n' \
  "$PROJECT_NUMBER" "$POOL_ID" "$PROVIDER_ID"
printf 'GCP_SERVICE_ACCOUNT_EMAIL=%s\n' "$SERVICE_ACCOUNT_EMAIL"
```

Only the two printed identifiers are GitHub variables, not secrets. The API
must be enabled in the service account's Cloud project. If the pool and
service account are in different projects, use the pool's project **number**
in the principal and provider name and the service account's own project in
its email and IAM operation.

## Play Console setup

1. In [Play Console](https://play.google.com/console), create or select the
   app and complete the applicable account, app and testing prerequisites.
   Choose the package ID before the first upload. If the app is new, upload an
   appropriately signed AAB manually first and configure Play App Signing;
   the upload action expects the app/package to exist already.
2. Under **Test and release > Setup > App signing**, record both the upload and
   Play app-signing certificate fingerprints. The configured keystore must
   contain the private key matching the registered **upload certificate**.
   Separately compare the Play app-signing certificate with the certificate
   of a side-loaded APK if updates across channels are required.
3. In **Users and permissions**, invite the Cloud service-account email and
   grant access to the specific app, including **View app information
   (read-only)** and **Release apps to testing tracks**. Add other permissions
   only if needed. This is separate from the Cloud WIF impersonation binding.
   See [Google Play Developer API setup](https://developers.google.com/android-publisher/getting_started).
4. Ensure the Play Console app's package ID matches the application ID the
   workflow uses: `ANDROID_PACKAGE_NAME` if set, otherwise the default
   `app.<owner>.<repo>`. The AAB application ID and the Play Console app's
   package ID must be identical. The GitHub WIF provider
   must trust the actual repository and branch, and the service-account email
   in GitHub must be the one invited to Play.

## Testing with Play

### Internal testing track (the current workflow)

In **Test and release > Testing > Internal testing**, create/select an email
list, add testers, set a feedback contact and save. Share the opt-in link;
testers must opt in with an eligible Google account. See
[Google's internal testing instructions](https://support.google.com/googleplay/android-developer/answer/9845334).

On the trusted branch, run **Actions > Auto-Build > Run workflow** and select
`android_play_store_upload`. Check the build artifact, signing identity,
upload job and Play Console release status. A completed internal-track release
can become available to opted-in testers; it is not merely an uncommitted
draft. First-time setup, review, tester access and propagation may delay
availability. The workflow does not automatically publish to production.

### Internal App Sharing (manual alternative)

This is not the internal testing track. In the app's **Test and release >
Setup > Internal app sharing**, configure authorized uploader and tester email
lists, or allow anyone with the link as appropriate. An authorized uploader
uses the [Internal App Sharing upload page](https://play.google.com/console/internal-app-sharing/)
to upload an APK or AAB and shares the resulting link. The local Play build
can supply the AAB, but neither the local script nor the current workflow
performs this upload.

On the tester's device, open Play Store settings, tap the Play Store version
seven times under **About**, then enable **Internal app sharing**. Open the
sharing link with the authorized account. Google re-signs these artifacts with
an Internal App Sharing test key; they generally cannot update installs signed
with another certificate. Version codes can be reused here; links expire
after 60 days and allow up to 100 downloads each. See
[Google's Internal App Sharing guide](https://support.google.com/googleplay/android-developer/answer/9844679).

## Troubleshooting

- **Cannot exchange GitHub token:** check `id-token: write`, the provider name
  and project number, issuer/audience, attribute mappings and conditions,
  repository/owner IDs, branch, and the service-account WIF binding. New IAM
  settings can take time to propagate.
- **Play API denied:** check that the Play Developer API is enabled and that
  the exact service-account email has the required app-level Play permissions.
  A successful Cloud authentication does not prove Play access.
- **Package or signing rejected:** compare `ANDROID_PACKAGE_NAME`, the
  bundle's application ID and the Play app ID; compare the keystore's
  certificate with Play's upload certificate. Do not confuse upload, Play
  app-signing and Internal App Sharing certificates.
- **Version code rejected:** compare against all existing releases and other
  publishers. GitHub run numbers are not globally coordinated.
- **Tester cannot install:** distinguish a track opt-in link from an Internal
  App Sharing link, check the Google account and tester list, and allow time
  for Play propagation. An incompatible installed signature requires
  uninstalling the old app before installing the new one; uninstalling can
  remove its local data.
