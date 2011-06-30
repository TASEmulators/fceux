--Input and frame counter display by andymac (by feos' request)
--Useful for recording input onto avi, or if you just don't want the 
--display getting in the way of your movie! 
--Simply drag and drop the input display or frame counter 
--Middle click to disable a display 
--Numpad 1-5 to enable displays. 

print("Input, frame & lag counter display by andymac (by feos' request).",
  "\r\nUseful for recording input & these counters onto avi.",
  "\r\nSimply drag and drop the input display or frame counter.",
  "\r\nMiddle click to disable a display.",
  "\r\nNumpad 1-4 to enable input displays.",
  "\r\nNumpad 5 to enable conters display.")

function drawpad(padnum,cx,cy)   -- draws a gamepad on the screen 

   gui.transparency (0)               --draws backround box 
   gui.drawbox (cx - 3,cy - 3,cx + 28,cy + 6,"blue")    
   gui.transparency (0) 

   gui.drawbox (cx,cy,cx + 3,cy + 3,"black")      --centre square 

   controller = joypad.read(padnum) 
    
   if (padnum == 1) then 
      colour = "red"                     --changes colour of gamepads 
   elseif (padnum == 2) then 
      colour = "yellow" 
   elseif (padnum == 3) then 
      colour = "green" 
   elseif (padnum == 4) then 
      colour = "orange" 
   end 

   if (controller.A) then 
      gui.drawbox (cx + 25,cy,cx + 28,cy + 3,colour)   --other buttons 
   else 
      gui.drawbox (cx + 25,cy,cx + 28,cy + 3,"black") 
   end 

   if (controller.B) then 
      gui.drawbox (cx + 20,cy,cx + 23,cy + 3,colour) 
   else 
      gui.drawbox (cx + 20,cy,cx + 23,cy + 3,"black") 
   end 

   if (controller.start) then 
      gui.drawbox (cx + 14,cy + 1,cx + 18,cy + 2,colour) 
   else 
      gui.drawbox (cx + 14,cy + 1,cx + 18,cy + 2,"black") 
   end 

   if (controller.select) then 
      gui.drawbox (cx + 8,cy + 1,cx + 12,cy + 2,colour) 
   else 
      gui.drawbox (cx + 8,cy + 1,cx + 12,cy + 2,"black") 
   end    

   if (controller.up) then 
      gui.drawbox (cx,cy - 3,cx + 3,cy,colour) 
   else 
      gui.drawbox (cx,cy - 3,cx + 3,cy,"black") 
   end 

   if (controller.down) then 
      gui.drawbox (cx,cy + 3,cx + 3,cy + 6,colour) 
   else 
      gui.drawbox (cx,cy + 3,cx + 3,cy + 6,"black") 
   end 

   if (controller.left) then 
      gui.drawbox (cx - 3,cy,cx,cy + 3,colour) 
   else 
      gui.drawbox (cx - 3,cy,cx,cy + 3,"black") 
   end 

   if (controller.right) then 
      gui.drawbox (cx + 3,cy,cx + 6,cy + 3,colour) 
   else 
      gui.drawbox (cx + 3,cy,cx + 6,cy + 3,"black") 
   end 


end 

function mouseover(boxx, boxy)                  --checks if mouseover 

   if keys.xmouse >= ( boxx - 5 ) and keys.xmouse <= ( boxx + 30 ) then 

      if keys.ymouse >= ( boxy -3 ) and keys.ymouse <= ( boxy + 21 ) then 

         return true 
      end 
   end 
end 

function inrange(upper, lower, testval) 

   if testval >= upper then return upper 

   elseif testval <= lower then return lower 

   else return testval 

   end 

end 

function everything() 

   keys = input.get() 

   if (text == 1) then 
       
      if (FCEU.lagged()) then      --flash lagcounter red if lagged, otherwise green. 
         gui.text (ex,ey + 9,FCEU.lagcount(), "red")       
      else 
         gui.text (ex,ey + 9,FCEU.lagcount(), "green") 
      end 
    
      if (movie.mode() == "finished" or movie.mode() == nil) then 
         gui.text (ex,ey,movie.framecount(), "red") 
      else 
         gui.text (ex,ey,movie.framecount()) 
      end 

   end 

    
   if keys["numpad5"] then text = 1 end         --enable 1-4 and text using and numpad 1-5 
   if keys["numpad1"] then pad1 = 1 end 
   if keys["numpad2"] then pad2 = 1 end 
   if keys["numpad3"] then pad3 = 1 end 
   if keys["numpad4"] then pad4 = 1 end 


   xmouse = inrange(240, 16, keys.xmouse)         --limits x and y mouse position to be on screen 

   ymouse = inrange(225, 11, keys.ymouse)         --so we can't drag items off the screen. 

   xmouse2 = inrange(241, 10, keys.xmouse) 

   ymouse2 = inrange(223, 16, keys.ymouse) 

   if keys.xmouse >= ( ex - 10 ) and keys.xmouse <= ( ex + 40 ) then      -- test if mouse is over text 

      if keys.ymouse >= ( ey ) and keys.ymouse <= ( ey + 40 ) then 

         motext = true 
      else 
         motext = false 
      end 
      else motext = false 
   end 
    

   if mouseover(pad1x, pad1y) then         -- checks if clicked, or middle clicked 

      if keys["leftclick"] then 
         pad1x = xmouse - 13 
         pad1y = ymouse - 8 
      elseif keys["middleclick"] then 
         pad1 = 0 
      end 
    
   elseif mouseover(pad2x, pad2y) then 

      if keys["leftclick"] then 
         pad2x = xmouse - 13 
         pad2y = ymouse - 8 
      elseif keys["middleclick"] then 
         pad2 = 0 
      end 

   elseif mouseover(pad3x, pad3y) then 

      if keys["leftclick"] then 
         pad3x = xmouse - 13 
         pad3y = ymouse - 8 
      elseif keys["middleclick"] then 
         pad3 = 0 
      end 

   elseif mouseover(pad4x, pad4y) then 

      if keys["leftclick"] then 
         pad4x = xmouse - 13 
         pad4y = ymouse - 8 
      elseif keys["middleclick"] then 
         pad4 = 0 
      end 

   elseif motext then 
      if keys["leftclick"] then 
         ex = xmouse2 - 10 
         ey = ymouse2 - 16 
      elseif keys["middleclick"] then 
         text = 0 
      end 
   end    

   if (pad1 == 1) then drawpad(1,pad1x, pad1y) end               --draw pads 
   if (pad2 == 1) then drawpad(2,pad2x, pad2y) end 
   if (pad3 == 1) then drawpad(3,pad3x, pad3y) end 
   if (pad4 == 1) then drawpad(4,pad4x, pad4y) end 

end 

while (true) do 

pad1x = 10 
pad1y = 200 
pad2x = 60 
pad2y = 200 
pad3x = 110 
pad3y = 200 
pad4x = 160 
pad4y = 200 
ex = 200 
ey = 200 
pad1 = 1 
pad2 = 0 
pad3 = 0 
pad4 = 0 
text = 1 

   while (true) do 

      gui.register(everything)    
      FCEU.frameadvance() 

   end 
end