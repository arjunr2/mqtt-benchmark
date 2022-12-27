import subprocess
import time
import shlex
import sys
import paho.mqtt.client as paho

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

# Return stdout if wait, else return proc instance
def deploy (cmd_list, devices, sync=False, wait=True):
    deploy_cmd = construct_deploy (cmd_list, devices, sync)
    print(deploy_cmd)
    proc = subprocess.Popen (deploy_cmd,
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=sys.stderr,
                            universal_newlines=True)
    if wait:
        stdout, stderr = proc.communicate()
        return stdout

    return proc


'''
    Callback for successful delivery of kill
'''
sync_var = False
def killpub_callback(client, userdata, mid):
    global sync_var
    print("Killing publish threads\n")
    sync_var = True

'''
    Safe killing of all publishers
    Publishes message to 'pubkill'
'''
def kill_pubs (broker_addr, port):
    global sync_var
    client = paho.Client("manager")
    client.connect(broker_addr, port)
    client.on_publish = killpub_callback
    client.loop_start()
    res, mid = client.publish ("pubkill", "kill", qos=1)
    if res:
        raise ValueError("Failure: Publish to pubkill")
    # Wait till kill signal is successfully delivered
    while not sync_var:
        pass
    # Sleep a bit for pub thread to completely kill itself
    time.sleep(0.5)
    client.loop_stop()
    client.disconnect()


