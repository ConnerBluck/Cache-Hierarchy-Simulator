
import os
import sys
import subprocess

config_dir = "./config"
traces_dir = "./traces"
executable_path = "./build/cachesim"
verification_out_filename = "verify.out"

# Get traces and configuration files
config_files = []
for dirpath,_, files in os.walk(config_dir):
    for f in files:
        fname = (os.path.abspath(os.path.join(dirpath, f)))
        if fname.endswith(".conf"):
            config_files.append(fname)

config_files.sort()

trace_files = []
for dirpath,_, files in os.walk(traces_dir):
    for f in files:
        fname = (os.path.abspath(os.path.join(dirpath, f)))
        if fname.endswith(".trace"):
            trace_files.append(fname)

trace_files.sort()

# Check if executable exists
if not os.path.exists(executable_path):
    sys.exit('The executable cachesim must exists at location ' + executable_path)

# Remove the verification file if it exists
if os.path.exists(verification_out_filename):
    os.remove(verification_out_filename)

f = open(verification_out_filename, "a+")

for conf in config_files:
    for trace in trace_files:
        run_args = [executable_path, "-c", conf, "-i", trace]
        cp = subprocess.run(run_args, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # Make sure there were no errors
        if len(cp.stderr) == 0:
            f.write('TRACE: ' + os.path.basename(trace) + '\n')
            f.write('CONF: ' + os.path.basename(conf) + '\n')
            f.write(cp.stdout)
            f.write('\n\n\n')

