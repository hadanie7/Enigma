/*
 * Copyright (C) 2016 Daniel Hadas
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

namespace manymouse {
    enum EventType {
        EVENT_MOTION,
        EVENT_BUTTON,
        EVENT_SCROLL
    };

    struct Event {
        EventType type;
        int player;
        int x;
        int y;
    };

    // these methods should be safe to use even when manymouse is off, and do nothing if not applicable.
    bool init(int number_of_players);
    void stop();
    bool poll_event(Event *event);
    void flush_events();
    int get_player_buttons(int iplayer);
    bool is_on();
}
