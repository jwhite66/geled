<?php

function run_cmd($c)
{
    unset($output);
    $last = exec($c, $output, $rc);
    $t = "Error - ";
    foreach ($output as $o)
        $t .= $o . " | ";
    if ($rc != 0)
        header("HTTP/1.0 500 " . $t);
    else
        echo "$last";

    return $rc;
}

function parse_message($line_array, &$on)
{
    $skip = true;
    $on = true;
    $out = "";
    foreach ($line_array as $l)
    {
        if (! $skip)
            $out .= $l;
        else
            if (preg_match("/NOT/", $l))
                $on = false;

        if (preg_match("/builtin message:/", $l))
            $skip = false;
    }

    return $out;
}

function handle_fifo()
{
    $fname = $_GET['fifo_name'];
    $verb = $_GET['fifo_verb'];
    $handle = fopen($fname, "a");
    if (! $handle)
    {
        header("HTTP/1.0 500 Error  - could not write {$verb} to {$fname}\n");
    }
    else
    {
        fwrite($handle, $verb . "\n");
        fclose($handle);
    }
}

    if (isset($_GET["cmd"]))
        $cmd = $_GET["cmd"];
    else
        $cmd = "none";

    if (isset($_GET["message"]))
        $message = $_GET["message"];

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
            $bmessage = parse_message($output, $on);
            if (strlen($bmessage) > 0 && $on)
            {
                $disp_status_desc = "On";
                $disp_status = "on";
            }
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
        run_cmd("../drive display --red=13 2>&1");
    }

    else if ($cmd == "displayoff")
    {
        run_cmd("../drive displayoff 2>&1");
    }

    else if ($cmd == "setmessage")
    {
        run_cmd("MESSAGE='{$message}' ../makehelper 2>&1");
    }

    else if ($cmd == "init")
    {
        $rc = run_cmd("../drive sync 2>&1");
        if ($rc == 0)
            $rc = run_cmd("../drive init 2>&1");
        if ($rc == 0)
            echo "Strings cleared (will also initialize).";
    }
    else if ($cmd == "fill")
    {
        run_cmd("../drive flood 2>&1");
        echo "Flooded all strings with color";
    }
    else if ($cmd == "chase")
    {
        run_cmd("../drive chase 2>&1");
    }
    else if ($cmd == "tetris")
    {
        run_cmd("DISPLAY=:0 ../tetrissim > /dev/null 2>&1 &");
    }
    else if ($cmd == "reset")
    {
        run_cmd("stty --file /dev/ttyUSB0  hupcl 2>&1 ; \
            (sleep 0.1 2>/dev/null || sleep 1) 2>&1 ; \
                stty --file /dev/ttyUSB0 -hupcl 2>&1 ");
        echo "Arduino reset.";
    }
    else if ($cmd == "fifo")
    {
        handle_fifo();
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
