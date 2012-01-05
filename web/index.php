<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
        "http://www.w3.org/TR/html4/loose.dtd">

<?php
    $g_title = "My test web page";
    include 'headers.php';

    $loading = '<div id="spinner_text">Loading</div><img id="spinner" src="loading.gif" alt="Loading">';
?>
     <script>
         function refresh_status()
         {
             $("#dstatus").html('<?php echo $loading;?>');
             $.get("webhelper.php?cmd=status", function(data){
                 $("#dstatus").html(data);
             });
         }
         $(document).ready(function(){
             $('.jtextfill').textfill({ maxFontPixels: 70, innerTag: 'a' });
             refresh_status();
         });

    </script>

    </head>

    <body>
        <div class="dynamicwhole">

            <?php include 'logo.php';?>

              <div class="tabdiv">
                <?php echo menu_html("Status"); ?>
              </div> <!--tabdiv-->

            <div class="content bg1">
                <div id="status_header">
                    <span>Status</span>
                    <div id="dstatus">
                    </div>
                </div>
                <a id="refreshbutton" href="#" onclick="refresh_status(); return false;" class="myButton">Refresh</a>
            </div> <!--content -->
        </div> <!--whole div -->

    </body>
</html>
