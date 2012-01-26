<?php

function run_cmd($c)
{
    $last = exec($c, $output, $rc);
    if ($rc != 0)
        header("HTTP/1.0 500 Error  - {$output[0]}");
    else
        echo "$last";

    return $rc;
}

    if (isset($_GET["cmd"]))
        $cmd = $_GET["cmd"];
    else
        $cmd = "none";

    if ($cmd == "status")
    {
        $template = file_get_contents("status.template");
        $template = str_replace('$status_title', "The Arduino did not respond properly", $template);
        $template = str_replace('$status_color', "errorcolor", $template);
        $template = str_replace('$status', "ERROR", $template);
        $template = str_replace('$display_status_description', "Off", $template);
        $template = str_replace('$display_status', "off", $template);
        $template = str_replace('$builtin_message', "[red]TC [green]MAKER", $template);

        sleep(1);
        echo $template;
    }
    else if ($cmd == "displayon")
    {
        run_cmd("../drive display");
    }

    else if ($cmd == "displayoff")
    {
        run_cmd("../drive displayoff");
    }

    else if ($cmd == "setmessage")
    {
        echo "Faking setmessage okay";
    }

    else if ($cmd == "init")
    {
        run_cmd("../drive init");
    }
    else if ($cmd == "fill")
    {
        run_cmd("../drive flood");
    }
    else if ($cmd == "chase")
    {
        run_cmd("../drive chase");
    }


    else
    {
        header("HTTP/1.0 500 Unknown command $cmd");
    }


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
