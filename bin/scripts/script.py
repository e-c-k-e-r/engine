# helper script to take a a folder of naive archive of backups and git-ify it
# can't be bothered to to change it from os to Pathlib

import os
import re
import subprocess
import py7zr
import shutil

from datetime import datetime

repo_dir = 'repo'
archive_dir = 'archives'
extract_dir = 'extracted'
archives = []

whitelist = []
blacklist = [ "dep/", "bin/" ]
blacklist_ext = [ ".o", ".dll", ".gdi", ".glb", ".spv" ]
whitelist_ext = [ ".lua", ".json", ".glsl", ".h" ]

# filter files to extract (that don't get .gitignore'd)
def valid_path_to_extract( f ):
	for w in whitelist:
		if w in f:
			return True
	
	for w in whitelist_ext:
		if f.endswith(w):
			return True

	for b in blacklist:
		if b in f:
			return False

	for b in blacklist_ext:
		if f.endswith(b):
			return False

	return True

# cleanup timestamp to one git likes
def format_git_datetime(dt):
	iso = dt.isoformat()
	if iso[-3] == ':' and iso[-6] == '+':
		return iso
	else:
		return iso[:-2] + ':' + iso[-2:]


# get timestamp from filename
for filename in os.listdir(archive_dir):
	# without time
	match = re.match(r'(\d{4})\.(\d{2})\.(\d{2})\.7z', filename)
	# with time
	if not match:
		match = re.match(r'(\d{4})\.(\d{2})\.(\d{2}) (\d{2})-(\d{2})-(\d{2})\.7z', filename)
	# no matches
	if not match:
		print(f'Skipping file: {filename}. Does not match expected filename format.')
		continue

	# set time to midnight
	if len(match.groups()) == 3:
		year, month, day = match.groups()
		hour, minute, second = '00', '00', '00'
	else:
		year, month, day, hour, minute, second = match.groups()
	
	# store it
	commit_datetime = datetime(int(year), int(month), int(day), int(hour), int(minute), int(second))
	archives.append((filename, commit_datetime))

# sort by date
archives.sort(key=lambda x: x[1])

# init repo
if not os.path.exists(repo_dir):
	os.makedirs(repo_dir)
subprocess.run(['git', 'init'], check=True, cwd=repo_dir)

# iterate (could probably use tqdm)
for filename, commit_datetime in archives:
	print(f'Processing: {filename}')

	# files to keep	
	items_to_keep = {'.git', '.gitignore', 'LICENSE'}
	for item in os.listdir(repo_dir):
		if item not in items_to_keep:
			item_path = os.path.join(repo_dir, item)
			if os.path.isfile(item_path):
				os.remove(item_path)
			elif os.path.isdir(item_path):
				shutil.rmtree(item_path)

	# create extraction dir
	if not os.path.exists(extract_dir):
		os.makedirs(extract_dir)

	# extract archive
	with py7zr.SevenZipFile(os.path.join(archive_dir, filename), mode='r') as z:
		z.extract(targets=[f for f in z.getnames() if valid_path_to_extract(f) ], path=extract_dir)
		
	archive_contents = os.listdir(extract_dir)

	# archive contains a folder of contents, copy from its contents
	if len(archive_contents) == 1 and os.path.isdir(os.path.join(extract_dir, archive_contents[0])):
		single_dir = os.path.join(extract_dir, archive_contents[0])
		for item in os.listdir(single_dir):
			s = os.path.join(single_dir, item)
			d = os.path.join(repo_dir, item)
			shutil.move(s, d)

		shutil.rmtree(single_dir)
	else:
		for item in archive_contents:
			s = os.path.join(extract_dir, item)
			d = os.path.join(repo_dir, item)
			shutil.move(s, d)

	# cleanup
	shutil.rmtree(extract_dir)

	# commit
	subprocess.run(['git', 'add', '.'], check=True, cwd=repo_dir)

	env = os.environ.copy()
	env['GIT_AUTHOR_DATE'] = format_git_datetime(commit_datetime)
	env['GIT_COMMITTER_DATE'] = format_git_datetime(commit_datetime)

	# might cause problems on no changes
	try:
		subprocess.run(['git', 'commit', '-m', f'Commit for {filename}'], env=env, check=True, cwd=repo_dir)
	except Exception as e:
		print(str(e))