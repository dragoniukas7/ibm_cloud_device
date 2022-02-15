map = Map("ibm_cloud_device")

section = map:section(NamedSection, "config", "ibm_cloud", "Config")

flag = section:option(Flag, "enable", "Enable", "Enable program")
orgId = section:option( Value, "orgId", "Organisation ID")
typeId = section:option( Value, "typeId" , "Type ID")
deviceId = section:option( Value, "deviceId", "Device ID")
token = section:option( Value, "token", "Authentication token")
token.datatype = "string"
token.maxlength = 32

return map
