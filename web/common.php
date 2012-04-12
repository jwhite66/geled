<?php

if (! isset($g_title))
    $g_title = "BlockLight Control";

function menu_html($selected_menu, $all_menus)
{
    $ret = "<ul>\n";
    foreach($all_menus as $m => $p)
    {
        $class="menu_item menufont bg1 tabwidth";
        $id="menu_{$m}";
        $onclick="onclick='
           $.get(\"{$p}\", function(data){
             $(\"#maincontent\").html(data);
            }); 

           $(\".menu_item\").removeClass(\"active\");
           $(\"#{$id}\").addClass(\"active\");
           
           return false;'";

        if ($selected_menu == $m)
            $class .= " active";
        $ret .= "<li id=\"{$id}\" class=\"{$class}\"><a {$onclick} href=\"#\">{$m}</a></li>\n";
    }
    $ret .= "</ul>\n";

    return $ret;
}

?>
