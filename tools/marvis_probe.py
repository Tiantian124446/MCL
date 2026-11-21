import os, sqlite3, shutil, glob, re

root = os.path.expandvars(r"%APPDATA%\Tencent\Marvis")
dbs = []
for pat in [r"db\*.db", r"electron_data\EasyControl\*.db"]:
    dbs += glob.glob(os.path.join(root, pat))
out = os.path.join(os.environ.get("TEMP", "."), "marvis_db_probe")
os.makedirs(out, exist_ok=True)
kw = re.compile(r"hotkey|shortcut|快捷|唤醒|alt\+|ctrl\+|window|浮窗|助手", re.I)
for db in dbs:
    name = os.path.basename(db)
    tmp = os.path.join(out, name)
    try:
        shutil.copy2(db, tmp)
    except Exception as e:
        print(f"COPY_FAIL {name} {e}")
        continue
    try:
        con = sqlite3.connect(tmp)
        cur = con.cursor()
        tables = [r[0] for r in cur.execute("select name from sqlite_master where type='table'")]
        print(f"=== {name} tables={tables}")
        for t in tables:
            try:
                cols = [r[1] for r in cur.execute(f'pragma table_info("{t}")')]
            except Exception:
                continue
            interesting_cols = [c for c in cols if re.search(r"key|name|value|config|data|json|text", c, re.I)]
            if not interesting_cols:
                continue
            try:
                rows = cur.execute(f'select * from "{t}" limit 3000').fetchall()
            except Exception:
                continue
            for row in rows:
                s = " | ".join(str(x) for x in row)
                if kw.search(s):
                    print(f"[{t}] {s[:500]}")
        con.close()
    except Exception as e:
        print(f"DB_FAIL {name} {e}")
