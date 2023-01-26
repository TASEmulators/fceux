
-- create a circle and put it in the center of the screen
while(true)
do
    --local circle = display.newCircle( display.contentWidth*0.5,display.contentHeight*0.5, 75)
   -- circle:setFillColor( 255 )
	gui.drawbox(30, 30, 60, 50)
    gui.drawbox(70, 70, 100, 90)
    gui.drawbox(30, 110, 60, 130)
    gui.drawbox(160, 70, 190, 100)
	FCEU.frameadvance();
    table tableOfInputs
   while(tableOfInputs = input.read())
   do
        if(tableOfInputs.leftclick)
        then
            if(tableOfInputs.xmouse == 30 && tableOfInputs.ymouse == 30 || 
        end
   end
end

-- touch listener function
--[[function circle:touch( event )
  if event.phase == "began" then
    -- first we set the focus on the object
    display.getCurrentStage():setFocus( self, event.id )
    self.isFocus = true

    -- then we store the original x and y position
    self.markX = self.x
    self.markY = self.y

  elseif self.isFocus then

    if event.phase == "moved" then
      -- then drag our object
      self.x = event.x - event.xStart + self.markX
      self.y = event.y - event.yStart + self.markY
    elseif event.phase == "ended" or event.phase == "cancelled" then
      -- we end the movement by removing the focus from the object
      display.getCurrentStage():setFocus( self, nil )
      self.isFocus = false
    end

  end

-- return true so Corona knows that the touch event was handled propertly
 return true
end

-- finally, add an event listener to our circle to allow it to be dragged
circle:addEventListener( "touch", circle )
end]]---