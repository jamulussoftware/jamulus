# Reporting Security Issues

**⚠️ Please do not open GitHub issues for security vulnerabilities. ⚠️**

We encourage responsible disclosure practices for security vulnerabilities.

If you think you have found a security-relevant issue, please send the details to **team@jamulus.io**.

We will then

- open a placeholder issue without details,
- assess the severity,
- work on a fix,
- schedule a release which includes the fix and
- publish all relevant details as part of the issue,
- publish a Github security advisory as necessary.

# Security model

## Guarantees

The Jamulus project aims to provide robust software.
It tries hard to avoid all kinds of implementation issues such as Code Execution, unwanted File system access, or similar issues.

## Limitations

The following is a list of areas where there may be expectations which Jamulus currently does not fulfill:

- There is no registration, authentication or user database.
  Usernames can freely be chosen.
- There is no encryption or message authentication.
  Both protocol messages and audio are transmitted in the clear.
  Anyone with access to the network traffic is able to listen to the audio or even modify it.
- There is little protection against certain patterns of Denial of Service:
  - Servers have a limited number of slots for clients which can easily be exhausted.
  - There is no protection against spam in the chat function.
  - Jamulus is UDP-based and UDP does not provide protection against source address spoofing. This enables all kinds of Denial of Service or Impersonation attacks.

For some of these areas there are long-term ideas about extending Jamulus (e.g. usage of TCP, authentication), but they have not been decided upon yet.
