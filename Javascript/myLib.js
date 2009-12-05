//Andres Delikat
//Javascript library for Javascript assignment 12
//Last update 10/18/2009

//Receives a number and returns its square
function calcSquare(num)
{
	return num*num
}

//Receives a message to display to the user in a prompt
//then asks the user until they enter a valid number
//then returns the number
function getNum(message)
{
	do
	{
		num = parseFloat(prompt(message))
	}   while (isNaN(num))
	
	return num
}

//Receives 3 paramters: the image to swap, the image to swap it to, and a message to put in the status bar
//image1 and image2 should be image objects
//status should be a string
//No return value
function swapImg(image1, image2, status)
{
	image1.src = image2.src		//Swap 1 with 2	
	var message = "Replaced " + image1.name + " with " + image2.name
	window.status = message
}

//Receives a variable number of paramters, and displays them as a numbered list
//No return value
function writeList()
{
	if (arguments.length < 1)
	{
		return
	}
	for (var x = 0; x < arguments.length; x++)
	{
		document.write(x, ": ", arguments[x], "<br>")
	}
}

