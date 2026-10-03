#!/usr/bin/env python3
"""Post-submission screening; never fetch attachments or execute submitted text."""
import json
import os
from pathlib import Path
import re
import urllib.request
from check_research_report import check, FULL_MAC

ATTACHMENT=re.compile(r'https?://(?:github\.com/user-attachments/|user-images\.githubusercontent\.com/|uploads\.github\.com/)',re.I)

def screen(body):
    problems=[]
    if len(body)>100000:return ['Submission too large for automatic screening.']
    if FULL_MAC.search(body):problems.append('A full MAC address appears in the submission.')
    if ATTACHMENT.search(body):problems.append('Attachments cannot be screened here; paste REDACTED JSON instead.')
    decoder=json.JSONDecoder();reports=[]
    for i,match in enumerate(re.finditer(r'\{',body)):
        if i>=128:
            problems.append('Too many JSON candidates; manual review needed.');break
        try:r,end=decoder.raw_decode(body[match.start():])
        except (ValueError,RecursionError):continue
        if isinstance(r,dict) and r.get('schema') in ('dnsp-device-research-v1','dnsp-device-research-v2'):reports.append(body[match.start():match.start()+end])
    if not reports:problems.append('No exported REDACTED report found in the issue body.')
    for text in reports:problems.extend(check(text))
    return list(dict.fromkeys(problems))

def main():
    event=json.loads(Path(os.environ['GITHUB_EVENT_PATH']).read_text())
    issue=event.get('issue',{})
    if issue.get('pull_request') or 'RESEARCH' not in issue.get('title','').upper():return
    # Current body avoids processing stale event snapshots after an edit.
    repo=os.environ['GITHUB_REPOSITORY'];number=int(issue['number'])
    if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+',repo):raise ValueError('Invalid repository')
    token=os.environ['GITHUB_TOKEN'];base='https://api.github.com/repos/'+repo
    def api(path,method='GET',data=None):
        request=urllib.request.Request(base+path,data=None if data is None else json.dumps(data).encode(),method=method,headers={'Authorization':'Bearer '+token,'Accept':'application/vnd.github+json','Content-Type':'application/json','X-GitHub-Api-Version':'2022-11-28'})
        with urllib.request.urlopen(request,timeout=20) as r:return json.load(r)
    current=api('/issues/'+str(number));body=current.get('body') or ''
    problems=screen(body)
    comment=event.get('comment',{})
    if comment and comment.get('user',{}).get('type')!='Bot':
        text=comment.get('body') or ''
        if FULL_MAC.search(text):problems.append('A full MAC appears in the new/edited comment.')
        if ATTACHMENT.search(text):problems.append('The comment contains an attachment that has not been screened.')
    if not problems:return # passing redaction is not research verification
    marker='<!-- dnsp-redaction-check -->'
    warning=marker+'\n**Redaction review needed.**\n\n'+ '\n'.join('- '+p for p in dict.fromkeys(problems))+'\n\nRetain PRIVATE locally. Submit only REDACTED JSON with MAC suffix XX:XX:XX and no advertised name. This check runs after posting and cannot retract uploaded attachments, notifications or edit history. Ask the maintainer for help if private data was exposed. No identification is automatically confirmed.'
    comments=api('/issues/'+str(number)+'/comments?per_page=100')
    if not any(c.get('user',{}).get('type')=='Bot' and c.get('body')==warning for c in comments):api('/issues/'+str(number)+'/comments','POST',{'body':warning})
if __name__=='__main__':main()
