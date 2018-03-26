<?php
	$machineFolder = $_GET['machineFolder'];
	$path = "C:\\xampp\\htdocs\\ProgramsExchange\\".$machineFolder."\\receivedFromMachine\\";
	$program_name = $_GET['program_name'];
	$extension = $_GET['extension'];
	
	if(file_exists($path.$program_name.$extension)){
		if(unlink($path.$program_name.$extension)){
			echo "File has been removed";
		}else{
			echo "Something went wrong";
		}
	}else{
		echo "Nothing happended, it's OK";
	}
?>