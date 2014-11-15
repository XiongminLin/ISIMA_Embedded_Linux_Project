<?php
header("Content-type:img/png");
$con = mysql_connect("localhost","root","123456");
	if(!$con){
		die('Could not connect to database:'.mysql_error());
		echo "fail";
	}
mysql_select_db("test", $con);
$result=mysql_query("SELECT * FROM temperature");
//$valueResult = mysql_query("SELECT * FROM temperature");
$storeNumber = mysql_num_rows($result);

function createImage($width,$height,$number,$arrayData){
  //定义坐标轴起始点坐标值；
   
  //X-起点
  $x1 = 40;
  $x2 = 360;
  
  
  //X-终点
  $x3 = 520;
  $x4 = 360;
  
  
  //Y-起点；
  $y1 = 40;
  $y2 = 20;
  
  
  //Y-终点
  $y3 = 40;
  $y4 = 360;
  
  
  //定义字号；
  $font = 10;
  
  $panel = imagecreate($width, $height);
  $red = imagecolorallocate($panel, 255, 0, 0);
  $green = imagecolorallocate($panel, 0, 222, 20);
  $white = imagecolorallocate($panel, 255, 255, 255);
  $black = imagecolorallocate($panel, 0, 0, 0);
  imagefill($panel, 0, 0, $black);  //填充背景；
  
  imageline($panel, $x1, $x2, $x3, $x4, $green);   //X轴
  imageline($panel, $y1, $y2, $y3, $y4, $green);   //Y轴
  
  
  imagestring($panel, $font, 15, 15, 'Temperature', $green);   //X字符
  imagestring($panel, $font, 520, 370, 'Time', $green);   //Y字符
  
  
  //与X轴平行的轴线；
  $step_x = 330; //定义第一条网线起始点纵坐标；
  for ($i=0; $i<10; $i++){
   imageline($panel, 40, $step_x, 510, $step_x, $green);
   $step_x -= 30;
  }
  
  //与Y轴平行的轴线；
  $step_y = 70; //定义第一条网线起始点横坐标；
  for ($i=0; $i<12; $i++){
   imageline($panel, $step_y, 35, $step_y, 360, $green);
   $step_y += 40;
  }
  
  //定义X轴参数；

$con = mysql_connect("localhost","root","123456");
  if(!$con){
    die('Could not connect to database:'.mysql_error());
    echo "fail";
  }
mysql_select_db("test", $con);
$y_init = mysql_query("SELECT * FROM temperature");
  //$y_point = array_values(range(1,12));
  //输出X轴参数；
  $x_str = 67;  //定义初始横坐标值；
  $font = 2;
  /*for($j = 0; $j<$number; $j++){
   imagestring($panel, $font, $x_str, 370, $y_point['hour'], $red);
   $x_str += 40;
  }*/
  $count = 0;
  $newData = array();
  $index = 0;
  while ($y_point = mysql_fetch_array($y_init)) {
    if ($count < $number) {
      imagestring($panel, $font, $x_str, 370, $y_point['hour'], $red);
      $x_str += 40;
      $count++;
    } 
     $real = floatval($y_point['value']);
        if ($real>0) {
          $newData[$index]=($real+20)/50;
          $index++;
        }elseif ($real<0) {
          $newData[$index]=(20-abs($real))/50;
          $index++;
        }elseif ($real==0) {
          $newData[$index]=0.4;
          $index++;
        }
  }
  
  //定义Y轴参数；
  $x_point = array(-20,-15,-10,-5,0,5,10,15,20,25,30);
  $y_str = 355;
  $font = 2;
  
  for($j = 0; $j<11; $j++){
   $xp = $x_point[$j];
   imagestring($panel, $font, 15, $y_str, $xp.'C', $red);
   $y_str -= 30;
  }
  
  
  //画折线
  $line_x = 70;  //横坐标of the start；
 // $data= array(0.83,0.87,0.96,0.8,1.0,0.92,0.2,0.00,0.00,0.00,0.00,0.00);
  for($r = 1; $r<$number; $r++){
   $line_y = 360 - $newData[$r-1] * 300;
   $line_y2 = 360 - $newData[$r] * 300;
   imageline($panel, $line_x, $line_y, $line_x + 40, $line_y2, $red);
   $line_x += 40;
  }
  
  imagepng($panel);
  imagedestroy($panel);
 }
 
 createImage(580, 400, $storeNumber,$arrayData);
 mysql_close($con);
?>