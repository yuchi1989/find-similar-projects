#!/usr/bin/python
import sys,os
import subprocess
classlist = []
typelist = []
includelist = []
headerlist = []

def analyze_file(filename, project_name, project_folder, output_file):
	global classlist
	global typelist
	global includelist
	global headerlist
	try:
		os.remove('project_api_info_temp.txt')
	except OSError:
		pass
	command = ""
	command = command + (os.path.expanduser('~/clang/build/bin/my_api_analyzer')) + ' '
	command = command + filename
	command = command + extraarg
	command = command + (' >project_api_info_temp.txt ')
	command = command + (' 2>&1 ')
	#print (command)
	subprocess.call(command,shell=True)
	with open('project_api_info_temp.txt') as tempfile:
		content = tempfile.readlines()
	for line in content:
		lline = line.split(',')
		if len(lline) >= 4 :
			if lline[3]!="" and project_folder in os.path.realpath(lline[0]):
				if lline[2]=="include":
					includelist.append(line)
				if lline[2]=="class":
					classlist.append(line)
				if lline[2]=="type":
					typelist.append(line)
				
		else:
			if project_folder in os.path.realpath(lline[0]):
				print("debug: " + filename + line)

if __name__ == '__main__':
	if len(sys.argv) != 4:
		print ("input error")
		print ("python3 project_api_analyzer.py project_name project_folder output_file")
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
			elif fname.lower().endswith(".hpp"):
				hppfilelist.append(fname)
				headerlist.append(f)
				if(hflag!=1):				
					hdir = root
					hdirectorylist[hdir] = 1
					for i in range(depth):
						hdir = hdir + "/../"
						hdirectorylist[os.path.normpath(hdir)] = 1
					hflag = 1
			elif fname.lower().endswith(".h"):
				headerlist.append(f)
				if(hflag!=1):				
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
	for f in hppfilelist:		
		analyze_file(f, sys.argv[1], os.path.realpath(sys.argv[2]), output_file)
	
	#output
	#print (len(includelist))
	for i in includelist:
		lline = i.split(',')
		filename = os.path.basename(lline[3]).strip()
		equal_flag = 0
		for j in headerlist:
			if j==filename:
				equal_flag=1
				break
		if equal_flag == 0:
			output_file.write(sys.argv[1] + ',' + i)

	for i in typelist:
		lline = i.split(',')
		typename = lline[3].strip()
		if "vector<" in typename or "map<" in typename:
			continue 
		if "const" in typename:
			typename = typename.replace("const"," ")
		if "static" in typename:
			typename = typename.replace("static"," ")
		if "volatile" in typename:
			typename = typename.replace("volaile "," ")
		if "mutable" in typename:
			typename = typename.replace("mutable "," ")
		if "*" in typename:
			typename = typename.replace("*"," ")
		if "&" in typename:
			typename = typename.replace("&"," ")
		typename = typename.strip()
		#print (typename)
		
		equal_flag = 0
		debug_flag=0
		for j in classlist:
			jline = j.split(',')
			jclassname = jline[3].strip()
			#print (jclassname)
			if jclassname in typename or typename in jclassname:
				equal_flag=1
				break
		
		if equal_flag == 0:
			output_file.write(sys.argv[1] + ',' + i)
		#print (equal_flag)
	output_file.close()