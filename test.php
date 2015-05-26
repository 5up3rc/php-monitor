<?php
date_default_timezone_set('PRC');
$host = "127.0.0.1:3306";
$user = "root";
$pass = "";
$database = "test";
mysql_connect($host, $user, $pass);
mysql_select_db($database);
$qry = "INSERT INTO `test` VALUES(NULL, '" . date("Y-m-d H:i:s") . "', 'xxxxxxxxxx');";
mysql_query($qry);