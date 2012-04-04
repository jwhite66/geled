<div id="tetris" class="insetcontent">
    <div id="tetris_control">
        <table width="100%">
        <tr width="100%">
           <td width="33%"></td>
           <td width="33%">
                <img id="rotate" src="rotate64.png" alt="Rotate">
           </td>
        </tr>

        <tr width="100%">
            <td width="33%">
                <img id="left" src="left64.png" alt="Left">
            </td>
            <td width="33%">
            </td>
            <td width="33%">
                <img id="right" src="right64.png" alt="Right">
            </td>
        </tr>

        <tr width="100%">
            <td width="33%">
            </td>
            <td width="33%">
                <img id="drop" src="drop64.png" alt="Fire">
            </td>
            <td width="33%"> </td>
        </tr>
        </table>
    </div>

    <div id="tetris_status">
        <div class="iconrow">
                   <div title="Start Tetris running" onclick="run_command('tetris');" class="button green">Start</div>
            <div class="score">Score: 0</div>
        </div>

        <div id="tetrisstatusrow" class="statusrow">
        </div>

    </div>
</div>

   <script src="jquery-1.6.4.js"></script>
   <script>
        function check_score()
        {
            $.ajax({
                url: 'webhelper.php?cmd=tetris.score',
                async : false,
                success: function( data ) 
                {
                    $(".score").html("Score: " + data);
                    console.log('score updated');
                },
                error: function( data, textStatus, errorThrown ) 
                {
                    $(".statusrow").removeClass("aokcolor");
                    $(".statusrow").addClass("errorcolor");
                    $(".statusrow").html(errorThrown);
                }
            });
        }

     $(document).ready(function(){
       $("img").click(function(event){
           send_command("fifo", "&fifo_name=myfifo&fifo_verb=" + event.target.id );
       });

       var interval_id = setInterval("check_score();", 1000);
     });
   </script>
