# Contributing device research

Reports begin **unverified** and describe an individual observed device. A user label is not an OUI-wide identification; detector confidence is not independent verification.

1. Open Research & Data > Device Research, hold a LOG row, and label/tag your observation. Raw-scan rows also support labeling.
2. Choose RESEARCH REPORT... and Export Both. microSD receives `/Research Submissions/report-XXXXXXXX-NNNN-REDACTED.txt` and its matching `/Research Submissions/PRIVATE/report-XXXXXXXX-NNNN-PRIVATE.txt`.
3. **Retain PRIVATE locally. Submit only REDACTED.** The MAC must end `XX:XX:XX`; advertised names are omitted. Review subtags and all free text yourself.
4. Optionally run `python3 tools/check_research_report.py /path/to/report-XXXXXXXX-NNNN-REDACTED.txt` offline.
5. Paste the REDACTED file contents into [the research form](https://github.com/your-dnsp/DNSquachWatch/issues/new?template=device_research.yml). Do not attach PRIVATE, raw captures, whole backups, or card images. The repository is public; a GitHub account is required to submit or comment.

Independent comments can describe your own equipment, physical inspection, repeatable observations, primary sources and uncertainty. Keep full addresses and personal details out of comments too. Maintainers record corroborated, confirmed or disputed status after reviewing evidence. No report automatically changes firmware identification rules.

## What the checks can and cannot do

The offline checker runs before sharing. The prepared GitHub workflow scans pasted issue text and newly edited comments **after posting**, and warns about complete MACs, unredacted report fields, or unscreened attachments. It never downloads attachments or executes submitted content. Passing redaction checks does not confirm device identity.

GitHub uploads an attachment as soon as it is attached. Issue-form prompts and after-posting checks cannot guarantee prevention, deletion, or recall of an upload, edit history, or notifications. If private data was shared accidentally, contact the maintainer promptly. Complete prevention would require a separate submission service that validates before publication; that is not part of this update.

## Source and documentation changes

For build requirements and checks, see [Build instructions](docs/BUILD.md). Keep changes focused, preserve DNSP menu organization and recovery behavior, and include the validation relevant to your change. Describe observed evidence and limitations when proposing detection rules. Never commit credentials, signing keys, private reports or personal device backups.

For independent research review, use the [corroboration guide](docs/research/CORROBORATION.md).
