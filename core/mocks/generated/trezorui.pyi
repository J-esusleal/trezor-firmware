from typing import *


# upymod/modtrezorui/modtrezorui-display.h
class Display:
    """
    Provide access to device display.
    """
    WIDTH: int  # display width in pixels
    HEIGHT: int  # display height in pixels

    def __init__(self) -> None:
        """
        Initialize the display.
        """

    def refresh(self) -> None:
        """
        Refresh display (update screen).
        """

    def bar(self, x: int, y: int, w: int, h: int, color: int) -> None:
        """
        Renders a bar at position (x,y = upper left corner) with width w and
        height h of color color.
        """

    def orientation(self, degrees: int | None = None) -> int:
        """
        Sets display orientation to 0, 90, 180 or 270 degrees.
        Everything needs to be redrawn again when this function is used.
        Call without the degrees parameter to just perform the read of the
        value.
        """

    def save(self, prefix: str) -> None:
        """
        Saves current display contents to PNG file with given prefix.
        """

    def clear_save(self) -> None:
        """
        Clears buffers in display saving.
        """
