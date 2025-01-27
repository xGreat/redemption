/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2012
 *   Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen,
 *              Meng Tan
 */

#include "mod/internal/widget/button.hpp"
#include "mod/internal/widget/label.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"
#include "gdi/draw_utils.hpp"
#include "utils/sugar/cast.hpp"
#include "utils/utf.hpp"
#include "keyboard/keymap.hpp"


WidgetButton::WidgetButton(
    gdi::GraphicApi & drawable,
    chars_view text, WidgetEventNotifier onsubmit,
    Color fg_color, Color bg_color, Color focus_color,
    unsigned border_width, Font const & font, int xtext, int ytext,
    bool logo)
: Widget(drawable, Focusable::Yes)
, auto_resize_(false)
, x_text(xtext)
, y_text(ytext)
, border_width(border_width)
, onsubmit(onsubmit)
, state(State::Normal)
, fg_color(fg_color)
, bg_color(bg_color)
, focus_color(focus_color)
, logo(logo)
, font(font)
{
    this->set_text(text);
}

WidgetButton::~WidgetButton() = default;

void WidgetButton::set_xy(int16_t x, int16_t y)
{
    Widget::set_xy(x, y);
    this->label_rect.x = x + (this->border_width - 1);
    this->label_rect.y = y + (this->border_width - 1);
}

void WidgetButton::set_wh(uint16_t w, uint16_t h)
{
    Widget::set_wh(w, h);
    this->label_rect.cx = w - (this->border_width * 2 - 1);
    this->label_rect.cy = h - (this->border_width * 2 - 1);
}

void WidgetButton::set_text(chars_view text)
{
    this->buffer[0] = 0;
    if (!text.empty()) {
        const size_t remain_n = buffer_size - 1;
        const size_t n = text.size();
        const size_t max = ((remain_n >= n) ? n : UTF8StringAdjustedNbBytes(text, remain_n));
        memcpy(this->buffer, text.data(), max);
        this->buffer[max] = 0;
        if (this->auto_resize_) {
            Dimension dm = WidgetLabel::get_optimal_dim(this->font, std::string_view(this->buffer), this->x_text, this->y_text);

            this->label_rect.cx = dm.w;
            this->label_rect.cy = dm.h;

            this->set_wh(this->label_rect.cx + (this->border_width * 2 - 1),
                         this->label_rect.cy + (this->border_width * 2 - 1));
        }
    }
}

void WidgetButton::rdp_input_invalidate(Rect clip)
{
    Rect rect_intersect = clip.intersect(this->get_rect());

    if (!rect_intersect.isempty()) {
        this->draw(
            rect_intersect, this->get_rect(), this->drawable, this->logo, this->has_focus, std::string_view(this->buffer),
            this->fg_color, this->bg_color, this->focus_color, gdi::ColorCtx::depth24(),
            this->label_rect, this->state, this->border_width, this->font, this->x_text, this->y_text);
    }
}

void WidgetButton::draw(
    Rect const clip, Rect const rect, gdi::GraphicApi& drawable,
    bool logo, bool has_focus, chars_view text,
    Color fg_color, Color bg_color, Color focuscolor, gdi::ColorCtx color_ctx,
    Rect label_rect, State state, unsigned border_width, Font const& font, int xtext, int ytext)
{
    if (label_rect.isempty()) {
        label_rect = rect;
        label_rect.x  += (border_width + 1);
        label_rect.y  += (border_width + 1);
        label_rect.cx -= (border_width * 2 + 1);
        label_rect.cy -= (border_width * 2 + 1);
    }

    if (state == State::Pressed) {
        label_rect.x++;
        label_rect.y++;
        label_rect.cx--;
        label_rect.cy--;
    }

    // Label color
    if (logo) {
        fg_color = has_focus ? focuscolor : fg_color;
    }
    else {
        bg_color = has_focus ? focuscolor : bg_color;
    }
    // background
    drawable.draw(RDPOpaqueRect(clip.intersect(rect), bg_color), rect, color_ctx);

    WidgetLabel::draw(clip, label_rect, drawable, text, fg_color, bg_color, color_ctx, font, xtext, ytext);

    if (logo) {
        return;
    }

    // border
    if (border_width) {
        gdi_draw_border(drawable, fg_color, rect, border_width, clip, color_ctx);
    }
}

void WidgetButton::rdp_input_mouse(uint16_t device_flags, uint16_t x, uint16_t y)
{
    if (device_flags == (MOUSE_FLAG_BUTTON1|MOUSE_FLAG_DOWN) && this->state == State::Normal) {
        this->state = State::Pressed;
        this->rdp_input_invalidate(this->get_rect());
    }
    else if (device_flags == MOUSE_FLAG_BUTTON1 && this->state == State::Pressed) {
        this->state = State::Normal;
        this->rdp_input_invalidate(this->get_rect());
        if (this->get_rect().contains_pt(x, y)) {
            this->onsubmit();
        }
    }
    else {
        this->Widget::rdp_input_mouse(device_flags, x, y);
    }
}

void WidgetButton::rdp_input_scancode(KbdFlags flags, Scancode scancode, uint32_t event_time, Keymap const& keymap)
{
    REDEMPTION_DIAGNOSTIC_PUSH()
    REDEMPTION_DIAGNOSTIC_GCC_IGNORE("-Wswitch-enum")
    switch (keymap.last_kevent()){
        case Keymap::KEvent::Enter:
            this->onsubmit();
            break;

        case Keymap::KEvent::KeyDown:
            if (keymap.last_decoded_keys().uchars[0] == ' ') {
                this->onsubmit();
            }
            break;

        default:
            Widget::rdp_input_scancode(flags, scancode, event_time, keymap);
            break;
    }
    REDEMPTION_DIAGNOSTIC_POP()
}

void WidgetButton::rdp_input_unicode(KbdFlags flag, uint16_t unicode)
{
    if (!bool(flag & KbdFlags::Release) && (unicode == ' ')) {
        this->onsubmit();
    }
    else {
        Widget::rdp_input_unicode(flag, unicode);
    }
}

Dimension WidgetButton::get_optimal_dim() const
{
    Dimension dm = WidgetLabel::get_optimal_dim(this->font, std::string_view(this->buffer), this->x_text, this->y_text);
    return Dimension(dm.w + (this->border_width * 2 - 1), dm.h + (this->border_width * 2 - 1));
}

Dimension WidgetButton::get_optimal_dim(unsigned border_width, Font const& font, chars_view text, int xtext, int ytext)
{
    Dimension dm = WidgetLabel::get_optimal_dim(font, text, xtext, ytext);
    return Dimension(dm.w + (border_width * 2 - 1), dm.h + (border_width * 2 - 1));
}
