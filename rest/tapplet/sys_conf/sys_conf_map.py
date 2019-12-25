from . conf_system import SystemConfTrans
from . conf_interfaces import IntfConfTrans
from . conf_actions import ActionsConfTrans
from . conf_acl import  AclConfTrans





sys_conf_map = {
    "system" : SystemConfTrans,
    "interfaces" : IntfConfTrans,
    "actions" : ActionsConfTrans,
    "acl" : AclConfTrans,
}
