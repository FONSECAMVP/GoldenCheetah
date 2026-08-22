#!/usr/bin/env python3
"""Bundled self-test for anti_duplication_guard.py. Run: python3 guard_selftest.py [guard_path]. Exit code = CORE failures."""
#!/usr/bin/env python3
import json, os, subprocess, sys, tempfile, shutil
GUARD = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "anti_duplication_guard.py")
def run(tool, tin, cwd):
    ev = json.dumps({"tool_name": tool, "tool_input": tin, "cwd": cwd})
    p = subprocess.run([sys.executable, GUARD], input=ev, capture_output=True, text=True, timeout=30)
    out = p.stdout.strip()
    if not out: return "passthrough"
    try: return json.loads(out)["hookSpecificOutput"]["permissionDecision"]
    except Exception: return "BADOUTPUT:"+out[:60]
def make_wiki_fixture():
    d = tempfile.mkdtemp(prefix="qgdw_wiki_")
    os.makedirs(os.path.join(d,"src","core"), exist_ok=True)
    os.makedirs(os.path.join(d,"wiki"), exist_ok=True)
    for f in ("prd.md","STATE.md","README.md","CONTRIBUTING.md"): open(os.path.join(d,f),"w").write("x")
    open(os.path.join(d,"prd.md.orig"),"w").write("orig")
    os.makedirs(os.path.join(d,"snap"), exist_ok=True)
    open(os.path.join(d,"snap","README.md.orig"),"w").write("orig")
    open(os.path.join(d,"WIKI.md"),"w").write(
        "# PROJECT WIKI\nroot: %s\nphase: 2\n\n## MAP\nprd.md req\nsrc/ code\nwiki/ brain\nSTATE.md cursor\nREADME.md ro\nCONTRIBUTING.md cg\n\n## REGISTRIES\nREQ 001-003 next:004\n\n## PAGES\nwiki/conventions.md conv\n" % d)
    return d
def make_nowiki_fixture():
    d = tempfile.mkdtemp(prefix="qgdw_nowiki_"); os.makedirs(os.path.join(d,"existing"), exist_ok=True); return d
W = make_wiki_fixture(); N = make_nowiki_fixture()
SK = os.path.join(W, ".claude", "skills", "someskill")
os.makedirs(SK, exist_ok=True)
open(os.path.join(SK, "SKILL.md"), "w").write("x")
# OUTSIDE the project, but its absolute path CONTAINS "/.claude/skills/" — the
# substring false positive. Containment must be decided BEFORE vendor policy.
XROOT = tempfile.mkdtemp(prefix="qgdw_outside_")
XSK = os.path.join(XROOT, ".claude", "skills", "example")
os.makedirs(XSK, exist_ok=True)
open(os.path.join(XSK, "snapshot.md"), "w").write("x")
# INSIDE the project but NOT the vendor tree: vendor territory is root-anchored, so
# this is an ordinary project path and normal MAP rules apply.
ISK = os.path.join(W, "scratch", ".claude", "skills")
os.makedirs(ISK, exist_ok=True)
CASES = [
 ("01","Bash",{"command":"mkdir src"},W,"deny","CORE","re-mkdir existing mapped dir"),
 ("02","Bash",{"command":"mkdir -p src"},W,"ask","CORE","mkdir -p no-op on existing"),
 ("03","Bash",{"command":"mkdir build"},W,"ask","CORE","new unmapped dir -> register"),
 ("04","Bash",{"command":"mkdir -p src/core/utils"},W,"passthrough","CORE","under mapped parent"),
 ("05","Bash",{"command":"echo hi > prd.md"},W,"deny","CORE","redirect clobber existing file"),
 ("06","Bash",{"command":"cat > prd.md <<EOF\nhi\nEOF"},W,"deny","CORE","heredoc redirect over existing"),
 ("07","Bash",{"command":"mkdir logs src"},W,"deny","CORE","multi-target, deny>ask precedence"),
 ("08","Bash",{"command":"mkdir src logs"},W,"deny","CORE","reversed order, deny still wins"),
 ("09","Bash",{"command":"touch prd.md"},W,"deny","CORE","touch existing file"),
 ("10","Bash",{"command":"touch newnote.txt"},W,"ask","CORE","touch NEW unmapped file -> ask-register"),
 ("11","Write",{"file_path": os.path.join(W,"prd.md")},W,"passthrough","CORE","Write=edit existing mapped"),
 ("12","Write",{"file_path": os.path.join(W,"notes.md")},W,"ask","CORE","new unmapped file -> register"),
 ("13","Write",{"file_path": os.path.join(W,"src","core","x.py")},W,"passthrough","CORE","new file under mapped dir"),
 ("14","Edit",{"file_path": os.path.join(W,"prd.md")},W,"passthrough","CORE","Edit always allowed"),
 ("15","Bash",{"command":"ls -la && grep foo prd.md"},W,"passthrough","CORE","no creation intent"),
 ("16","Bash",{"command":"mkdir /tmp/qgdw_outofroot_xyz"},W,"passthrough","CORE","path outside project root -> leave alone"),
 ("17","Bash",{"command":"install -d src"},W,"passthrough","GAP","install -d intentionally not parsed"),
 ("18","Bash",{"command":"cp README.md prd.md"},W,"deny","CORE","cp clobber existing file"),
 ("19","Bash",{"command":"mkdir \"my dir\""},W,"ask","CORE","quoted new dir -> ask (tokenizer ok)"),
 ("20","Bash",{"command":"cat > brandnew.md <<EOF\nx\nEOF"},W,"ask","CORE","redirect-create NEW file -> ask"),
 ("21","Bash",{"command":"mkdir existing"},N,"deny","CORE","no-WIKI clobber still denies"),
 ("22","Bash",{"command":"mkdir freshdir"},N,"passthrough","CORE","no-WIKI new dir -> silent"),
 ("23","Bash",{"command":"mv README.md prd.md"},W,"deny","CORE","mv clobber existing file"),
 ("24","Bash",{"command":"tee README.md < /dev/null"},W,"deny","CORE","tee clobber existing file"),
 ("25","Bash",{"command":"ln -sf WIKI.md README.md"},W,"deny","CORE","ln -sf overwrite existing"),
 ("26","Bash",{"command":"cp README.md NEWCOPY.md"},W,"ask","CORE","cp to NEW unmapped file -> ask"),
 ("27","Bash",{"command":"cp prd.md src/core"},W,"passthrough","CORE","cp INTO existing dir -> ok (no false deny)"),
 ("28","Bash",{"command":"mv prd.md src/core/prd.md"},W,"passthrough","CORE","mv dest under mapped dir -> ok"),
 ("29","Bash",{"command":"tee newlog.txt"},W,"ask","CORE","tee NEW file -> ask"),
 ("30","Bash",{"command":"install -d src"},W,"passthrough","GAP","scaffolder/install -d gap (sibling of 17)"),
 ("31","Bash",{"command":"echo hi >> prd.md"},W,"passthrough","CORE","append >> to existing preserves content — NOT a clobber"),
 ("32","Bash",{"command":"tee -a README.md"},W,"passthrough","CORE","tee -a append to existing — NOT a clobber"),
 ("33","Bash",{"command":"cp prd.md.orig prd.md"},W,"passthrough","CORE","snapshot restore (LSN-032) is sanctioned"),
 ("34","Bash",{"command":"mv prd.md.orig prd.md"},W,"passthrough","CORE","snapshot restore via mv is sanctioned"),
 ("35","Bash",{"command":"grep \"ln -sf\" src"},W,"passthrough","CORE","verb inside a quoted PATTERN is data, not a command"),
 ("36","Bash",{"command":"cat > brandnew3.md <<EOF\ncp README.md prd.md\nEOF"},W,"ask","CORE","heredoc BODY mentioning cp is data; only the redirect-create counts"),
 ("37","Bash",{"command":"echo hi >> brandnewlog.md"},W,"ask","CORE","append creating a NEW unmapped file -> ask-register"),
 ("38","Bash",{"command":"tee README.md < /dev/null"},W,"deny","CORE","tee WITHOUT -a still truncates -> deny"),
 ("39","Bash",{"command":"cp snap/README.md.orig README.md"},W,"passthrough","CORE","SCRATCHPAD restore (cross-dir basename match) is sanctioned — LSN-032 field flow"),
 ("40","Bash",{"command":"cp snap/README.md.orig prd.md"},W,"deny","CORE","backup of a DIFFERENT file onto existing is still a clobber"),
 ("V1","Write",{"file_path": os.path.join(SK,"newtool.py")},W,"deny","CORE","new file in .claude/skills -> vendor-territory deny"),
 ("V2","Write",{"file_path": os.path.join(SK,"SKILL.md")},W,"ask","CORE","edit skill file -> lost-on-update warning"),
 ("V3","Bash",{"command":"mkdir .claude/skills/someskill/newdir"},W,"deny","CORE","mkdir in skills tree -> vendor deny"),
 ("V4","Write",{"file_path": os.path.join(XSK,"new.md")},W,"passthrough","CORE","NEW Write OUTSIDE project whose path contains /.claude/skills/ -> passthrough"),
 ("V5","Edit",{"file_path": os.path.join(XSK,"snapshot.md")},W,"passthrough","CORE","EDIT existing file OUTSIDE project under a /.claude/skills/ path -> passthrough"),
 ("V6","Bash",{"command":"cp prd.md %s/copy.md" % XSK},W,"passthrough","CORE","Bash creation OUTSIDE project under /.claude/skills/ -> passthrough"),
 ("V7","Bash",{"command":"mkdir %s/newdir" % XSK},W,"passthrough","CORE","Bash mkdir OUTSIDE project under /.claude/skills/ -> passthrough"),
 ("V8","Write",{"file_path": os.path.join(ISK,"file.md")},W,"ask","CORE","internal scratch/.claude/skills is NOT vendor territory -> ordinary MAP ask"),
 # P-series: quote context inside COMMAND SUBSTITUTIONS. `$( )` opens a fresh parsing
 # context, so a flat scan desyncs on the first inner `"` and reads quoted DATA as a
 # command. Both directions matter: data must pass, real nested commands must not.
 ("P1","Bash",{"command":'''printf "%s -> %s\\n" "label" "$(probe '{"tool_input":{"command":"mkdir .claude/skills/newthing"},"cwd":"/x"}')"'''},W,"passthrough","CORE","nested substitution, single-quoted JSON arg is DATA -> passthrough"),
 ("P2","Bash",{"command":'''printf "%s -> %s\\n" "label" "$(probe '{"tool_input":{"command":"mkdir .claude/skills/newthing"},"cwd":"'"$PWD"'"}')"'''},W,"passthrough","CORE","same shape with the $PWD quote splice -> still DATA -> passthrough"),
 ("P3","Bash",{"command":'echo "$(mkdir .claude/skills/newthing)"'},W,"deny","CORE","command GENUINELY executed inside $( ) is still scanned -> vendor deny"),
 ("P4","Bash",{"command":"mkdir .claude/skills/newthing"},W,"deny","CORE","direct project-root vendor mkdir -> deny (unchanged)"),
 ("P5","Bash",{"command":'''echo '{"command":"mkdir .claude/skills/newthing"}' | cat'''},W,"passthrough","CORE","simple single-quoted JSON containing mkdir is DATA -> passthrough"),
 ("P6","Bash",{"command":'echo "$(mkdir %s/subst_newdir)"' % XSK},W,"passthrough","CORE","real nested mkdir OUTSIDE project under a /.claude/skills/ path -> passthrough"),
 # NB: the chain must stay in the event's cwd. `cd /tmp && mkdir src` would really create
 # /tmp/src, so denying it would codify the guard's separate inability to model in-command
 # cwd changes rather than the substitution scanning this case exists to prove.
 ("P7","Bash",{"command":'echo "$(true && mkdir src)"'},W,"deny","CORE","chained command inside $( ) is scanned; recreating the mapped src dir -> deny"),
 ("P8","Bash",{"command":'''echo "outer $(echo "inner $(mkdir .claude/skills/deep)") tail"'''},W,"deny","CORE","doubly-nested substitution: real mkdir still found"),
 ("P9","Bash",{"command":'''echo "outer $(echo 'inner mkdir .claude/skills/deep') tail"'''},W,"passthrough","CORE","doubly-nested, inner is single-quoted DATA -> passthrough"),
 ("P10","Bash",{"command":'echo "no substitution here: mkdir src"'},W,"passthrough","CORE","plain double-quoted string is DATA (simple-quote behavior preserved)"),
]
rows=[]; core_fail=0; gap_fail=0
for cid,tool,tin,cwd,exp,klass,note in CASES:
    act = run(tool,tin,cwd); ok=(act==exp); status="PASS" if ok else "FAIL"
    if not ok:
        if klass=="CORE": core_fail+=1
        else: gap_fail+=1
    rows.append((cid,klass,status,exp,act,note))
p_empty = subprocess.run([sys.executable,GUARD],input="",capture_output=True,text=True)
p_bad   = subprocess.run([sys.executable,GUARD],input="not json",capture_output=True,text=True)
empty_ok = (p_empty.stdout.strip()=="" and p_empty.returncode==0)
bad_ok   = (p_bad.stdout.strip()=="" and p_bad.returncode==0)
if not empty_ok: core_fail+=1
if not bad_ok: core_fail+=1
print("CASE | CLASS | STATUS | EXPECTED | ACTUAL | NOTE")
print("-----|-------|--------|----------|--------|-----")
for cid,klass,status,exp,act,note in rows:
    print(f"{cid}   | {klass:4} | {status:4} | {exp:11}| {act:11}| {note}")
print(f"S1   | CORE | {'PASS' if empty_ok else 'FAIL'} | passthrough| {'passthrough' if empty_ok else 'OUTPUT'} | empty stdin must not block")
print(f"S2   | CORE | {'PASS' if bad_ok else 'FAIL'} | passthrough| {'passthrough' if bad_ok else 'OUTPUT'} | malformed stdin must not block")
print("\nSUMMARY_JSON="+json.dumps({"core_failures":core_fail,"gap_failures":gap_fail}))
shutil.rmtree(W,ignore_errors=True); shutil.rmtree(N,ignore_errors=True); shutil.rmtree(XROOT,ignore_errors=True)
sys.exit(core_fail)
