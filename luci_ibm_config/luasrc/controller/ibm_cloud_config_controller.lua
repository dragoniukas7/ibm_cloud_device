module("luci.controller.ibm_cloud_config_controller", package.seeall)

function index()
	entry({"admin", "services", "ibm_cloud_config"}, cbi("ibm_cloud_model"), _("IBM cloud config"),105)
end
