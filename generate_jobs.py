import argparse
import sys
import os
import glob
import stat

minEvent = 0
maxEvent = 16755464 # default number
jobName = "TTjob"
outputName = "TTout"
configFile = "config.ini"
directory = ""
outputDir = ""

def query_yes_no(question, default="yes"):
	# credit to http://stackoverflow.com/a/3041990
	valid = {"yes": True, "y": True, "ye": True, "no": False, "n": False}
	if default is None:
		prompt = " [y/n] "
	elif default == "yes":
		prompt = " [Y/n] "
	elif default == "no":
		prompt = " [y/N] "
	else:
		raise ValueError("invalid default answer: '%s'" % default)
	
	while True:
		sys.stdout.write(question + prompt)
		choice = raw_input().lower()
		if default is not None and choice == '':
			return valid[default]
		elif choice in valid:
			return valid[choice]
		else:
			sys.stdout.write("Please respond with 'yes' or 'no' (or 'y' or 'n').\n")

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='Generates N jobs as *.sh files.')
	parser.add_argument('-j', action='store', dest='jobs', help='number of jobs to be generated')
	parser.add_argument('--min-event', action='store', dest='min_event', help='min event (default 0)')
	parser.add_argument('--max-event', action='store', dest='max_event', help='max event (16755464)')
	parser.add_argument('--job-name', action='store', dest='job_name', help='prefix of the job script *.sh name (TTjob)')
	parser.add_argument('--output', action='store', dest='output', help='prefix of the *.root output file name (TTout)')
	parser.add_argument('--dir', action='store', dest='dir', help='directory of the *.sh files')
	parser.add_argument('--output-dir', action='store', dest='output_dir', help='direcotry of the *.root output files')
	parser.add_argument('-v', action='store_true', dest='verbose', help='enables verbose mode in the job program')
	parser.add_argument('--config-file', action='store', dest='config', help='specifies config file for the program (config.ini)')
	results = parser.parse_args()
	
	j_parsed = results.jobs
	if(j_parsed == None):
		parser.error('You have to specify the number of jobs.')
	
	Nmin = minEvent if results.min_event == None else int(results.min_event)
	Nmax = maxEvent if results.max_event == None else int(results.max_event)
	j = int(j_parsed)
	
	if(j < 2):
		parser.error('Number of jobs must be greater than 1.')
	if(j > Nmax - Nmin):
		parser.error('Number of jobs cannot exceed the number of events.')
	if(Nmax < 0 or Nmin < 0):
		parser.error('Event number must be positive.')
	if(Nmax < Nmin):
		parser.error('Max cannot be smaller than min.')
	
	N = Nmax - Nmin
	doDivide = (N % j == 0)
	if(not doDivide): j = j - 1
	incr = N / j
	doAdd = False
	if(not doDivide): doAdd = (Nmax - j*incr < incr / 2)
	ranges = []
	for i in range(0, j):
		start = Nmin + i * incr
		end = Nmin + (i + 1) * incr
		if(i == j - 1 and doAdd):
			ranges.append([start, Nmax])
			break
		ranges.append([start, end])
	if(doAdd):
		print "You had only ", Nmax - j * incr, " events for the last job,"
		print "so they were added to the next-to-last job."
	print "Start at: ", minEvent
	print "End at: ", maxEvent
	print "Number of events per job: ", incr
	if(not doDivide):
		print "Number of events for the last job: ",
		print (ranges[len(ranges) - 1][1] - ranges[len(ranges) - 1][0])
	print "Total number of jobs: ", len(ranges)
	if(j > 5):
		print "%d) [%d,%d]" % (1,ranges[0][0],ranges[0][1])
		print "%d) [%d,%d]" % (2,ranges[1][0],ranges[1][1])
		print "\t..."
		print "%d) [%d,%d]" % (len(ranges) - 1,ranges[len(ranges) - 2][0],ranges[len(ranges) - 2][1])
		print "%d) [%d,%d]" % (len(ranges),ranges[len(ranges) - 1][0],ranges[len(ranges) - 1][1])
	else:
		for i in range(len(ranges)):
			print "%d) [%d,%d]" % (i + 1,ranges[i][0],ranges[i][1])
	doContinue = query_yes_no("Proceed?")
	if not doContinue: sys.exit()
	
	jobName = results.job_name if results.job_name != None else jobName
	outputName = results.output if results.output != None else outputName
	directory = results.dir if results.dir != None else directory
	outputDir = results.output_dir if results.output_dir != None else outputDir
	configFile = results.config if results.config != None else configFile
	enableVerbose = results.verbose
	
	pattern = jobName + "_*.sh"
	if(directory != ""): pattern = directory + "/" + pattern
	oldFiles = glob.glob(pattern)
	if len(oldFiles) != 0:
		print "Found", len(oldFiles), "files matching the pattern " + pattern
		doDelete = query_yes_no("Delete them?")
		if doDelete:
			for path in oldFiles:
				if os.path.isfile(path):
					os.remove(path)
			print "Deleted", len(oldFiles),"files."
	if(directory != ""):
		if not os.path.exists(directory):
			os.makedirs(directory)
	for i in range(len(ranges)):
		filename = jobName + "_" + str(i + 1) + ".sh"
		if(directory != ""): filename = directory + "/" + filename
		outputFilename = outputName + "_" + str(i + 1)
		file = open(filename, 'w+')
		file.write("#!/bin/bash\n")
		file.write("./")
		file.write("process.out")
		file.write(" -I ")
		file.write(configFile)
		file.write(" -b ")
		file.write(str(ranges[i][0]))
		file.write(" -e ")
		file.write(str(ranges[i][1]))
		file.write(" -o ")
		file.write(outputFilename)
		if(outputDir != ""):
			file.write(" -d ")
			file.write(outputDir)
		if(enableVerbose):
			file.write(" -v ")
		file.write("\n")
		file.close()
		st = os.stat(filename)
		os.chmod(filename, st.st_mode | stat.S_IEXEC)
	print "Generated", len(ranges), "files."
		
	
	
	
