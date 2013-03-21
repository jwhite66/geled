<div id="signboard" class="insetcontent">
   <script src="jquery-1.6.4.js"></script>
   <script type="text/javascript">
        function get_message()
        {
            $("#signboard_process_list").html("<p>Fetching message...</p>");
            $.ajax({
                url: 'webhelper.php?cmd=getmessage',
                async : false,
                success: function( data ) 
                {
		    if (data.length == 0)
                        $("#signboard_process_list").html("<p>No message displaying</p>");
                    else
                        $("#signboard_process_list").html("<p>Current message:<br>" + data +"</p>");
                },
                error: function( data, textStatus, errorThrown ) 
                {
                    alert("Error setting message: " + errorThrown);
                }
            });
        }
        function change_message()
        {
            $.ajax({
                url: 'webhelper.php?cmd=runmessage&message=' +
                    encodeURIComponent($('#signboard_message_text').val()),
                async : false,
                success: function( data ) 
                {
     		    get_message();
                },
                error: function( data, textStatus, errorThrown ) 
                {
                    alert("Error setting message: " + errorThrown);
                }
              });
        }
   </script>

   <div id="signboard_message_body">
   <textarea id="signboard_message_text" title="Enter the message here.  A special square bracked syntax can be used to start a color change.  For example, [red]TEXT, will make red TEXT.  Advanced colors can be done with 4 hex digits like this: [bb|rr|gg|bb] where bb is bright in hex, rr is red in hex, gg green, and bb blue.  For example, [CC|0F|00|00] is bright red." value="Your message here">Your message here</textarea>
   <input id="signboard_message_button" value="Start signboard with message" type="submit" onclick="change_message(); return false;"></input>
   </div>
   <div id="signboard_process_list">
   <p>Signboard process list will go here</p>
   </div>
   <div id="signboard_message_control">
        <div title="Stop Signboard" onclick="run_command('stopmessage'); get_message();" class="button red">Stop Signboard</div>
   </div>

</div>

 <script>
     get_message();
 </script>

