<?php
    $dstatus = '<h1>Hi mom</h1>';
    sleep(5);
    echo $dstatus;
/*
    $fp = popen("ps -C ledscrollsim -o pid=,args=", "r");
    while ($fp && !feof($fp))
    {
       $l = fgets($fp);
       $msgpids .= $l;
    }
    pclose($fp);
*/
?>
