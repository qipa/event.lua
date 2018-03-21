




local event = require "event"
local mysql_core = require "mysql.core"

local CREATE_TABLE_SQL = [[
CREATE TABLE IF NOT EXISTS `test2_tbl`(
   `id` INT UNSIGNED AUTO_INCREMENT,
   `title` VARCHAR(100) NOT NULL,
   `author` VARCHAR(40) NOT NULL,
   `submission_date` DATE,
   PRIMARY KEY ( `id` )
)ENGINE=InnoDB DEFAULT CHARSET=utf8;
]]

local mysql,err = mysql_core.connect("127.0.0.1","root","2444cc818a3bbc06","event_test",3306)
print(mysql,err)
table.print(mysql:execute(CREATE_TABLE_SQL))
table.print(mysql:execute("insert into test1_tbl (title,submission_date) values (\"mrq\",123456)"))

table.print(mysql:execute("select id from test1_tbl where id = 1"))

table.print(mysql:execute("insert into test1_tbl (title) values (\"mrq\")"))

table.print(mysql:execute("update test1_tbl set title='mrq1',submission_date = 18-03-20 where id = 1"))

table.print(mysql:execute("select * from test1_tbl where id = 1"))