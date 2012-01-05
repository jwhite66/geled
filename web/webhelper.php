<?php
    $dstatus = 'Status placeholder';
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
