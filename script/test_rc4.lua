local util = require "util"


local code = util.authcode("mrq","fuck",1)
print(util.authcode(code.."1","fuck",0))