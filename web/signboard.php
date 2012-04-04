<div id="signboard" class="insetcontent">
   <script src="jquery-1.6.4.js"></script>
   <script type="text/javascript">
        function change_message()
        {
            $.ajax({
                url: 'webhelper.php?cmd=runmessage&message=' +
                    encodeURIComponent($('#signboard_message_text').val()),
                async : false,
                success: function( data ) 
                {
                    /*alert('Message changed');*/
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
   <div id="signboard_message_control">
        <div title="Stop Signboard" onclick="run_command('stopmessage');" class="button red">Stop Signboard</div>
   </div>

</div>
