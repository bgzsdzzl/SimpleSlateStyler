# Contributing to SimpleSlateStyler

Thanks for taking the time to contribute.

## Before You Start

- For bug reports and feature requests, use the issue templates.
- For anything larger than a one-line fix, open an issue first so we can
  agree on the approach before you write code.

## Development Setup

1. Fork and clone the repository.
2. Place the plugin inside a UE 5.8 project under `Plugins/SimpleSlateStyler/`.
3. Regenerate project files and build the editor target.
4. Verify the plugin loads by running `SimpleSlateStyler.Dump` in the editor
   console — it should print every entry from `Config/SimpleSlateStyler/`.

## Development Workflow

```bash
git checkout -b feature/my-feature
# make changes
python3 Scripts/check_syntax.py Source
git commit -s -m "Add my feature"
git push origin feature/my-feature
```

Then open a Pull Request against `main`.

## Developer Certificate of Origin (DCO)

Every commit in a pull request must be signed off. This project uses the
[Developer Certificate of Origin](https://developercertificate.org/)
rather than a Contributor License Agreement.

Add the sign-off with `git commit -s`, or enable it globally:

```bash
git config --global format.signOff true
```

The sign-off line must appear at the end of every commit message:

```
Signed-off-by: Your Name <your.email@example.com>
```

If you forget, amend the commit:

```bash
git commit --amend --signoff
git push --force-with-lease
```

The `DCO` status check on GitHub will fail until every commit in the PR
is signed off.

## Code Style

- Follow the Unreal Engine coding standard.
- Tabs for indentation in C++ sources.
- Keep comments in English.
- One feature per pull request.

## What CI Checks

Every pull request must pass:

- **DCO** — every commit is signed off.
- **PR Checks** — no `Binaries/`, `Intermediate/` or `Saved/` directories
  committed; `Scripts/check_syntax.py` passes; the PR description mentions
  how the change was verified.

You can run the syntax check locally before pushing:

```bash
python3 Scripts/check_syntax.py Source
```

## Pull Request Checklist

- [ ] Commit messages are signed off (`git commit -s`).
- [ ] No compiled output is committed.
- [ ] Local build succeeds for the editor target.
- [ ] DSL changes are documented in `Docs/DSL.md`.
- [ ] API changes are documented in `Docs/API.md`.
- [ ] `CHANGELOG.md` has an entry under `[Unreleased]`.

## License

By contributing to SimpleSlateStyler, you agree that your contributions
will be licensed under the Apache License, Version 2.0 (see [LICENSE](LICENSE)).

When modifying an existing file, add a line to the file header:

```
// Modified by <Your Name> on YYYY-MM-DD
```

## Reporting Security Issues

Do not open a public issue for security problems. Contact the maintainer
directly at the address in the `NOTICE` file.