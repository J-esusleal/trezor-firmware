from __future__ import annotations

import typing as t
from enum import Enum

from trezorlib.debuglink import LayoutType

from .. import buttons
from .. import translations as TR

if t.TYPE_CHECKING:
    from trezorlib.debuglink import DebugLink, LayoutContent

    AllActionsType = t.List[t.Union[str, t.Tuple[str, ...]]]


# Passphrases and addresses for both models
class CommonPass:
    RANDOM_25 = "Y@14lw%p)JN@f54MYvys@zj'g"
    RANDOM_25_ADDRESS = "mnkoxeaMzLgfCxUdDSZWrGactyJJerQVW6"

    SHORT = "abc123ABC_<>"
    SHORT_ADDRESS = "mtHHfh6uHtJiACwp7kzJZ97yueT6sEdQiG"

    WITH_SPACE = "abc 123"
    WITH_SPACE_ADDRESS = "mvqzZUb9NaUc62Buk9WCP4L7hunsXFyamT"

    EMPTY_ADDRESS = "mvbu1Gdy8SUjTenqerxUaZyYjmveZvt33q"


class PassphraseCategory(Enum):
    MENU = "MENU"
    DIGITS = "123"
    LOWERCASE = "abc"
    UPPERCASE = "ABC"
    SPECIAL = "#$!"


def get_char_category(char: str) -> PassphraseCategory:
    """What is the category of a character"""
    if char.isdigit():
        return PassphraseCategory.DIGITS
    if char.islower():
        return PassphraseCategory.LOWERCASE
    if char.isupper():
        return PassphraseCategory.UPPERCASE
    return PassphraseCategory.SPECIAL


def go_next(debug: "DebugLink") -> LayoutContent:
    if debug.layout_type is LayoutType.TT:
        return debug.click(buttons.OK)
    elif debug.layout_type is LayoutType.TR:
        return debug.press_right()
    elif debug.layout_type is LayoutType.Mercury:
        return debug.swipe_up()
    else:
        raise RuntimeError("Unknown model")


def tap_to_confirm(debug: "DebugLink") -> LayoutContent:
    if debug.layout_type is LayoutType.TT:
        return debug.read_layout()
    elif debug.layout_type is LayoutType.TR:
        return debug.read_layout()
    elif debug.layout_type is LayoutType.Mercury:
        return debug.click(buttons.TAP_TO_CONFIRM)
    else:
        raise RuntimeError("Unknown model")


def go_back(debug: "DebugLink", r_middle: bool = False) -> LayoutContent:
    if debug.layout_type in (LayoutType.TT, LayoutType.Mercury):
        return debug.click(buttons.CANCEL)
    elif debug.layout_type is LayoutType.TR:
        if r_middle:
            return debug.press_middle()
        else:
            return debug.press_left()
    else:
        raise RuntimeError("Unknown model")


def navigate_to_action_and_press(
    debug: "DebugLink",
    wanted_action: str,
    all_actions: AllActionsType,
    is_carousel: bool = True,
    hold_ms: int = 0,
) -> None:
    """Navigate to the button with certain action and press it"""
    # Orient
    try:
        _get_action_index(wanted_action, all_actions)
    except ValueError:
        raise ValueError(f"Action {wanted_action} is not supported in {all_actions}")

    def current_action() -> str:
        return layout.get_middle_choice()

    def current_is_wanted(wanted_action: str) -> bool:
        # Allowing for possible multiple actions on one button
        return (
            current_action() == wanted_action
            or current_action() in wanted_action.split("|")
        )

    # Navigate
    layout = debug.read_layout()
    while not current_is_wanted(wanted_action):
        layout = _move_one_closer(
            debug=debug,
            wanted_action=wanted_action,
            current_action=current_action(),
            all_actions=all_actions,
            is_carousel=is_carousel,
        )

    # Press or hold
    debug.press_middle(hold_ms=hold_ms)


def unlock_gesture(debug: "DebugLink") -> LayoutContent:
    if debug.layout_type is LayoutType.TT:
        return debug.click(buttons.OK)
    elif debug.layout_type is LayoutType.TR:
        return debug.press_right()
    elif debug.layout_type is LayoutType.Mercury:
        return debug.click(buttons.TAP_TO_CONFIRM)
    else:
        raise RuntimeError("Unknown model")


def _get_action_index(wanted_action: str, all_actions: AllActionsType) -> int:
    """Get index of the action in the list of all actions"""
    if wanted_action in all_actions:
        return all_actions.index(wanted_action)
    for index, action in enumerate(all_actions):
        if not isinstance(action, tuple):
            action = (action,)
        for subaction in action:
            try:
                tr = TR.translate(subaction)
            except KeyError:
                continue
            if tr == wanted_action:
                return index

    raise ValueError(f"Action {wanted_action} is not supported in {all_actions}")


def _move_one_closer(
    debug: "DebugLink",
    wanted_action: str,
    current_action: str,
    all_actions: AllActionsType,
    is_carousel: bool,
) -> LayoutContent:
    """Pressing either left or right regarding to the current situation"""
    index_diff = _get_action_index(wanted_action, all_actions) - _get_action_index(
        current_action, all_actions
    )
    if not is_carousel:
        # Simply move according to the index in a closed list
        if index_diff > 0:
            return debug.press_right()
        else:
            return debug.press_left()
    else:
        # Carousel can move in a circle - over the edges
        # Always move the shortest way
        action_half = len(all_actions) // 2
        if index_diff > action_half or -action_half < index_diff < 0:
            return debug.press_left()
        else:
            return debug.press_right()
