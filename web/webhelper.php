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

function parse_message($line_array)
{
    $skip = true;
    $out = "";
    foreach ($line_array as $l)
    {
        if (! $skip)
            $out .= $l;
        if (preg_match("/builtin message:/", $l))
            $skip = false;
    }

    return $out;
}

    if (isset($_GET["cmd"]))
        $cmd = $_GET["cmd"];
    else
        $cmd = "none";

    if ($cmd == "status")
    {
        $template = file_get_contents("status.template");
        $disp_status_desc = "Off";
        $disp_status = "off";
        $bmessage = "";

        $last = exec("../drive status", $output, $rc);
        if ($rc != 0)
        {
            $template = str_replace('$status_title', "The Arduino did not respond properly", $template);
            $template = str_replace('$status_color', "errorcolor", $template);
            $template = str_replace('$status', "ERROR", $template);
        }
        else
        {
            $bmessage = parse_message($output);
            $template = str_replace('$status_title', $output[0], $template);
            $template = str_replace('$status_color', "aokcolor", $template);
            $template = str_replace('$status', "OK", $template);
        }

        $template = str_replace('$display_status_description', $disp_status_desc, $template);
        $template = str_replace('$display_status', $disp_status, $template);
        $template = str_replace('$builtin_message', $bmessage, $template);

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
