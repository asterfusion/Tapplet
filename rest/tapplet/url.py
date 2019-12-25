from tornado.log import app_log

from tapplet.system_handler import sf_system_urls

from tapplet.interface_handler import interfaces_urls
from tapplet.actions.action_handler import sf_action_urls
from tapplet.acl_handler import acl_urls

from tapplet.vpp_handler import vpp_cmd_urls

tapplet_urls = (
    vpp_cmd_urls,
    sf_system_urls,
        interfaces_urls,
	sf_action_urls,
    acl_urls,

            )

