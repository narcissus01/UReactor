#!/usr/bin/env python3
"""Vendor Context from an official Boost source archive, without extracting all Boost.
Usage: python tools/vendor_boost_context.py D:/boost_1_92_0.tar.gz
Refuses to overwrite an existing destination. No network or build tools required.
"""
import argparse, hashlib, json, re, tarfile
from pathlib import Path

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('archive', type=Path)
    parser.add_argument('--output', type=Path, default=Path(__file__).resolve().parents[1] / 'third_party/boost_context')
    args = parser.parse_args()
    if args.output.exists():
        raise SystemExit('Destination exists; use a new --output directory and review before replacing.')
    digest = hashlib.file_digest(args.archive.open('rb'), 'sha256').hexdigest()
    with tarfile.open(args.archive, 'r|gz') as archive:
        # Stream gzip once; random tar seeks repeatedly decompress the archive.
        members = {}
        for member in archive:
            name = member.name
            if member.isfile() and ('/boost/' in name or name.endswith('/CMakeLists.txt') or
                                    '/cmake/' in name or '/libs/context/src/' in name or
                                    name.endswith('/LICENSE_1_0.txt')):
                members[name] = archive.extractfile(member).read()
        root = next(n[:-len('boost/version.hpp')] for n in members if n.endswith('/boost/version.hpp'))
        def read(name):
            return members[root + name]
        modules, pending = {}, ['context']
        while pending:
            module = pending.pop()
            if module in modules: continue
            content = read(f'libs/{module}/CMakeLists.txt').decode()
            modules[module] = sorted(set(re.findall(r'Boost::([a-zA-Z0-9_]+)', content)) - {module})
            pending.extend(x for x in modules[module] if x not in modules)
        # Keep whole dependency header directories: config/predef use computed includes.
        headers = {n[len(root):] for n in members if n.startswith(root+'boost/') and
                   any(n.startswith(root+'boost/'+m+'/') for m in modules)}
        headers.add('boost/version.hpp')
        # Public umbrella headers and legacy forwarding headers live outside module dirs.
        headers.update('boost/'+m+'.hpp' for m in modules if root+'boost/'+m+'.hpp' in members)
        work = list(headers)
        files = {}
        while work:
            name = work.pop()
            data = read(name)
            files['include/'+name] = data
            for dependency in re.findall(rb'#\s*include\s*[<"](boost/[^>"\r\n]+)[>"]', data):
                dep = dependency.decode()
                if root+dep in members and dep not in headers:
                    headers.add(dep);work.append(dep)
        for module in modules:
            for name in members:
                rel = name[len(root):]
                if rel == f'libs/{module}/CMakeLists.txt' or rel.startswith(f'libs/{module}/cmake/') or (module=='context' and rel.startswith('libs/context/src/')):
                    files[rel] = read(rel)
        files['LICENSE_1_0.txt'] = read('LICENSE_1_0.txt')
        for rel, data in files.items():
            path = args.output / rel
            if not path.resolve().is_relative_to(args.output.resolve()): raise ValueError(rel)
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        manifest = {'archive':args.archive.name, 'sha256':digest, 'modules':modules,
                    'files':{name:hashlib.sha256(data).hexdigest() for name,data in sorted(files.items())}}
        (args.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8',newline='\n')
        print(json.dumps({'modules':sorted(modules),'files':len(files),'bytes':sum(map(len,files.values())), 'sha256':digest},indent=2))
if __name__ == '__main__': main()
