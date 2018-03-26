<?php
	$machineFolder = $_GET['machineFolder'];
	$path = "C:\\xampp\\htdocs\\ProgramsExchange\\".$machineFolder."\\receivedFromMachine\\";
	$program_name = $_GET['program_name'];
	$extension = $_GET['extension'];
	$line = $_GET['line'];
	
	$file = fopen($path.$program_name.$extension, "a") or die ("Unable to open or create a file!");
	
	$line = str_replace("~", " ", $line);
	
	fwrite($file, $line);
	fwrite($file, chr(13));
	fwrite($file, chr(10));
	fclose($file);
	
	echo $line;
	
	echo "a line has been added";
?>