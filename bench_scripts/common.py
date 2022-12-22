'''
    Common helpers for benchmark creation
'''

def construct_deploy (cmd_list, devices, sync=False):
    command_str = '; '.join(cmd_list)
    sync_str = "--sync" if sync else ""
    device_str = f"--devices {' '.join(devices)}" 
    deploy_cmd = "python3 /home/hc/silverline/hc/manage.py "\
                 "--config /home/hc/silverline/hc/config/hc.cfg "\
                f"{sync_str} {device_str} -x \"{command_str}\""
    return deploy_cmd

