<?php

if (! isset($g_title))
    $g_title = "BlockLight Control";

$g_menus = array("Status" => "status.php", "Control" => "control.php",
                 "Apps" => "apps.php", "SpaceWar" => "spacewar.php");

function menu_html($selected_menu)
{
    global $g_menus;
    $ret = "<ul>\n";
    foreach($g_menus as $m => $p)
    {
        $class="menufont bg1 tabwidth";
        $onclick="onclick='
           $.get(\"{$p}\", function(data){
             $(\"#maincontent\").html(data);
            }); return false;'";

        if ($selected_menu == $m)
            $class .= " active";
        $ret .= "<li class=\"{$class}\"><a {$onclick} href=\"#\">{$m}</a></li>\n";
    }
    $ret .= "</ul>\n";

    return $ret;
}

?>

