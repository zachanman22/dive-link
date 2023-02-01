import pygame
import json


# Define some colors.
BLACK = pygame.Color('black')
WHITE = pygame.Color('white')


# This is a simple class that will help us print to the screen.
# It has nothing to do with the joysticks, just outputting the
# information.
class TextPrint(object):
    def __init__(self):
        self.reset()
        self.font = pygame.font.Font(None, 20)

    def tprint(self, screen, textString):
        textBitmap = self.font.render(textString, True, BLACK)
        screen.blit(textBitmap, (self.x, self.y))
        self.y += self.line_height

    def reset(self):
        self.x = 10
        self.y = 10
        self.line_height = 15

    def indent(self):
        self.x += 10

    def unindent(self):
        self.x -= 10


pygame.init()

# Set the width and height of the screen (width, height).
screen = pygame.display.set_mode((500, 700))

pygame.display.set_caption("My Game")

# Loop until the user clicks the close button.
done = False

# Used to manage how fast the screen updates.
clock = pygame.time.Clock()

# Initialize the joysticks.
pygame.joystick.init()

# Get ready to print.
textPrint = TextPrint()


heading = 0
headingGain = 1

def updateHeading(axisValue):
    global heading, headingGain
    axisValue = axisValue*headingGain
    heading += axisValue
    while heading >= 255:
        heading -= 255
    while heading < 0:
        heading += 255


def mapping(axesData):
    # Left Joystick
    # Axis 0: Left/Right [-1,1]
    # Axis 1: Up/Down [-1,1]
    # Right Joystick
    # Axis 2: Left/Right [-1,1]
    # Axis 3: Up/Down [-1,1]
    # Triggers
    # Axis 4: Right Trigger, open to full pressed [-1,1]
    # Axis 5: Left Trigger, open to full pressed [-1,1]
    forward = 0
    up = 0
    updateHeading(axis[0])
    if(axis[4] > -0.99):
        forward = -(axis[4]+1)/2
    else:
        forward = (axis[5]+1)/2
    forward = int(round(forward*7))

    up = int(round(-1*axis[3]*7))

    return {"h":int(heading)%255,"f":forward,"u":up}

# -------- Main Program Loop -----------
while not done:
    #
    # EVENT PROCESSING STEP
    #
    # Possible joystick actions: JOYAXISMOTION, JOYBALLMOTION, JOYBUTTONDOWN,
    # JOYBUTTONUP, JOYHATMOTION
    if pygame.QUIT in [event.type for event in pygame.event.get()]:
        done = True
    # for event in pygame.event.get(): # User did something.
    #     if event.type == pygame.QUIT: # If user clicked close.
    #         done = True # Flag that we are done so we exit this loop.

    #
    # DRAWING STEP
    #
    # First, clear the screen to white. Don't put other drawing commands
    # above this, or they will be erased with this command.
    screen.fill(WHITE)
    textPrint.reset()

    # Get count of joysticks.
    joystick_count = pygame.joystick.get_count()

    # For each joystick:
    for i in range(joystick_count):
        joystick = pygame.joystick.Joystick(i)
        joystick.init()

        axes = joystick.get_numaxes()
        axis = [joystick.get_axis(i) for i in range(axes)]
        value = mapping(axis)
        strval = json.dumps(value)

        textPrint.tprint(screen, "Data {}".format(strval))
        textPrint.unindent()

        buttons = joystick.get_numbuttons()
        textPrint.tprint(screen, "Number of buttons: {}".format(buttons))
        textPrint.indent()

        buttonData = [joystick.get_button(i) for i in range(buttons)]
        print(buttonData)
        for i in range(buttons):
            button = joystick.get_button(i)

        
    #
    # ALL CODE TO DRAW SHOULD GO ABOVE THIS COMMENT
    #

    # Go ahead and update the screen with what we've drawn.
    pygame.display.flip()

    # Limit to 20 frames per second.
    clock.tick(20)

# Close the window and quit.
# If you forget this line, the program will 'hang'
# on exit if running from IDLE.
pygame.quit()
