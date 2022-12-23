import subprocess
import shlex
import sys

'''
    Common helpers for benchmark creation
'''

def construct_deploy (cmd_list, devices, sync=False):
    command_str = '; '.join(cmd_list)
    sync_str = "--sync" if sync else ""
    device_str = f"--devices {' '.join(devices)}" 
    deploy_cmd = "python3 /home/hc/silverline/hc/manage.py "\
                 "--config /home/hc/silverline/hc/config/hc.cfg "\
                f"cmd {sync_str} {device_str} -x \"{command_str}\""
    return deploy_cmd

def deploy (cmd_list, devices, sync=False):
    deploy_cmd = construct_deploy (cmd_list, devices, sync)
    print(deploy_cmd)
    proc = subprocess.Popen (deploy_cmd,
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=sys.stderr,#subprocess.PIPE,
                            universal_newlines=True)
    stdout, stderr = proc.communicate()
    print(stdout)
    #with open("output.txt", "w") as f:
    #    f.write(stdout)

