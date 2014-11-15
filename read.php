<html>
<body>
<center>
<?php 
header("Content-type:text/html;charset=utf-8");
$con = mysql_connect("localhost","root","123456");
	if(!$con){
		die('Could not connect to database:'.mysql_error());
		echo "fail";
	}
mysql_select_db("test", $con);
$result=mysql_query("SELECT * FROM temperature");
//$valueResult = mysql_query("SELECT value FROM temperature");
echo "The information about the temperature :";
echo "<br>";
echo "<br>";
echo "<table border='1'>
<tr>
<th>Temperature(c)</th>
<th>Time</th>
<th>Date</th>
</tr>";
  $newData = array();
  $index = 0;
while($row=mysql_fetch_array($result)){
	echo "<tr>";
	echo "<th>".$row['value']."</th>";
	echo "<th>".$row['hour']."</th>";
	echo "<th>".$row['date']."</th>";
	echo "</tr>";
//   $newData[$index]= $row['value'];
//   $index = $index+1;
//    echo $index." ";
}
//$storeNumber = mysql_num_rows($result);
//echo "all the contents is".$storeNumber;
//  for ($i=0; $i < 7; $i++) { 
//  	echo " ".$newData[$i]."<br>";  }
mysql_close($con);
echo "</table>";
?>
<br>
<img src="read1.php">
</center>
</body>
<html>