<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
        "http://www.w3.org/TR/html4/loose.dtd">

<?php
    $g_title = "Blocklight Control";
    $g_menus = array("Status" => "status.php", "Control" => "control.php",
                     "Signboard" => "signboard.php", "Lights" => "lights.php",
                     "SpaceWar" => "spacewar.php", "Tetris" => "tetris.php");

    include 'common.php';
    include 'headers.php';
?>
    </head>

<script>
     $(document).ready(function(){
         $('.jtextfill').textfill({ maxFontPixels: 70, innerTag: 'span' });
         $.get("status.php", function(data){
             $("#maincontent").html(data);
         });
     });
</script>

    <body>
        <div class="fixedwhole">
            <div class="lights">
                <img src="logo.png">
            </div>

              <div class="tabdiv">
                <?php echo menu_html("Status", $g_menus); ?>
              </div> <!--tabdiv-->

            <div id="maincontent" class="content bg1">
            </div> <!--content -->

        </div> <!--whole div -->

    </body>
</html>
