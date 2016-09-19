#!/usr/bin/python
import sys,os
import subprocess

def analyze_file(filename, project_name, project_folder, output_file):
	try:
		os.remove('project_info_temp.txt')
	except OSError:
		pass
	command = ""
	command = command + (os.path.expanduser('~/clang/build/bin/my_analyzer')) + ' '
	command = command + filename
	command = command + extraarg
	command = command + (' >project_info_temp.txt ')
	command = command + (' 2>&1 ')
	#print (command)
	subprocess.call(command,shell=True)
	with open('project_info_temp.txt') as tempfile:
		content = tempfile.readlines()
	for line in content:
		lline = line.split(',')
		if len(lline) >= 4 :
			#if lline[3]!="" and project_folder in os.path.realpath(lline[0]):
				output_file.write(project_name + ',' + line)
		else:
			#if project_folder in os.path.realpath(lline[0]):
				print("debug: " + filename + line)

if __name__ == '__main__':
	if len(sys.argv) != 4:
		print ("input error")
		print ("project_analyzer.py project_name project_folder output_file")
		sys.exit(2)
	#traverse project folder, look for .cc .c files and determine the directory that including .h
	cfilelist=[]
	ccfilelist=[]
	cppfilelist=[]
	hppfilelist=[]
	hdirectorylist={}
	extraarg=" -- "   
	print (sys.argv[2])
	for root, dirs, files in os.walk(sys.argv[2]):
		hflag = 0
		for f in files:         
			fname = os.path.join(root, f)
			depth = len(root.split('/')) - len(sys.argv[2].split('/'))
			if fname.lower().endswith(".c"):
				cfilelist.append(fname)
			elif fname.lower().endswith(".cc"):
				ccfilelist.append(fname)            
			elif fname.lower().endswith(".cpp"):
				cppfilelist.append(fname)
			elif fname.lower().endswith(".h") or fname.lower().endswith(".hpp"):
				if(hflag==1):
					continue
				#print (fname)
				hdir = root
				hdirectorylist[hdir] = 1
				for i in range(depth):
					hdir = hdir + "/../"
					hdirectorylist[os.path.normpath(hdir)] = 1
				hflag = 1
	for i in hdirectorylist:
		extraarg = extraarg + " -I" + i + " "
	print (len(hdirectorylist))
	
	#analyze each file with clang libtool my_analyzer
	output_file = open(sys.argv[3],'a')	
	for f in cfilelist:		
		analyze_file(f, sys.argv[1], os.path.realpath(sys.argv[2]), output_file)
	for f in ccfilelist:		
		analyze_file(f, sys.argv[1], os.path.realpath(sys.argv[2]), output_file)
	for f in cppfilelist:		
		analyze_file(f, sys.argv[1], os.path.realpath(sys.argv[2]), output_file)
	output_file.close()