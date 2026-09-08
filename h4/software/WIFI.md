# WiFi configuration and firmware updates

The controller can bring up its own WiFi network so you can edit every setting from a phone or
laptop and install new firmware without carrying a computer to the lathe and plugging in a cable.

It is **off by default**. With it off there is no radio, no extra task, and the controller behaves
exactly as it did before this existed. Nothing here happens unless you turn it on.

---

## Turning it on

All of this is done at the machine — the radio has to be switched on from the panel before there is
any web page to switch it on from.

1. Press **settings**, scroll to **WiFi & updates**, press **play** to open it.
2. On **Access PIN**, type an 8-digit number and press **play**. This is the WiFi password.
   It must be exactly 8 digits — see [Why 8 digits](#why-the-pin-is-eight-digits) below.
3. On **Enabled**, press **play** to switch it to `yes`.

**No restart needed.** The access point comes up within a second or so, the splash line shows
`NanoEls-H4`, and the **Enabled** item shows the address beside its value:

```
WiFi & updates 1/2
Enabled
Now yes   192.168.4.1
ON to toggle
```

Switching it back to `no` shuts the radio down again, also without a restart. Turning it on to
configure the machine and off again afterwards is a perfectly reasonable way to work.

If it shows `WiFi failed to start`, the PIN is the first thing to check — the setting switches
itself back off rather than retrying, so the controller is never stuck in a failing loop.

Changing the PIN while the network is up restarts it so the new password takes effect, which
disconnects anyone currently on it. That is the point of changing it.

## Connecting

Join the WiFi network **`NanoEls-H4`** using your PIN as the password, then browse to
**http://192.168.4.1**.

Your phone will most likely warn that this network has no internet connection and offer to switch
back to mobile data. Tell it to stay connected, or the page will not load.

---

## What you can do

### Derived figures

The first panel on the page is read-only. It works out what your settings actually *mean*, because
the consequence of a setting is often invisible in the setting itself:

| Figure | Why it matters |
|---|---|
| **Resolution** | The smallest movement one motor step produces. Nothing finer than this can be expressed — a backlash value below it does nothing. |
| **Steps/mm** | The same fact in the units other motion controllers use. |
| **Max feed** | How fast the axis can actually travel, from its max step rate. |
| **Stops in** | How far it still travels while decelerating from full speed. Worth knowing before putting a soft limit near the chuck. |
| **Backlash** | Compensation expressed in motor steps. If this reads `0 (none)` the setting is too small to have any effect and is highlighted. |
| **Max rpm** | **The important one.** The fastest the spindle can turn at the pitch you have set before the axis can no longer keep up and the thread loses sync. |
| **Encoder limit** | The rpm above which the glitch filter starts discarding real pulses as noise, at the encoder and at the spindle. |

The two sentences under the table state the two ceilings in plain language, so you do not have to
read the table to get the number that matters.

Everything here updates as soon as you change a setting that feeds it.

### Settings

Every value in the LCD menu appears on the page, grouped into the same sections and in the same
order — the page is generated from the same table the controller's own menu walks, so the two can
never disagree.

- **Numbers** — type the new value.
- **Distances** — shown in mm or inches depending on the controller's current measurement mode.
  Type `0.42`, not `4200`.
- **Toggles** — tap to switch.

**Nothing is sent to the controller until you press Save.** Edited rows are highlighted and marked
with a dot, and a bar appears at the bottom of the screen showing how many changes are waiting:

```
2 unsaved changes          [ Discard ]  [ Save ]
```

This is deliberate. A half-typed number never reaches the machine, and values that belong together
— a lead screw pitch and the motor steps that go with it — commit as one action rather than
leaving the machine briefly configured with one of the two. **Discard** puts everything back.
Pressing Enter in any field saves the lot.

Changes are written one at a time rather than all at once, because each one takes the controller's
motion lock and commits to flash. If one is rejected the rest still go through, and the failed row
keeps your value with the reason in red underneath so you can fix it and save again.

The same validation runs as when you type a value on the keypad — the web page cannot store
anything the menu would have refused.

Closing the tab with unsaved changes prompts you first, since staged edits live only in the
browser.

### Backup

**Download settings** saves every value as a text file. Restore it by pasting the lines back over
USB serial. This is worth doing before any firmware update.

### Firmware update

1. Build the sketch and find the `.bin` (in Arduino IDE: *Sketch → Export compiled binary*).
2. Choose the file, press **Install**, and watch the progress bar.
3. The controller writes the image, restarts itself, and comes back on the new version. Check the
   splash screen shows the version you expect.

The LCD shows `Updating firmware` and the kilobytes written while this happens.

---

## Safety

**The web interface cannot move the machine.** There is no jog, no zero, no start. The only things
it can do are change stored settings and install firmware. Motion is only ever commanded from the
controller's own keypad.

**Nothing can be changed while the machine is running.** If the controller is on, or an axis is
moving, both settings writes and firmware updates are refused and the page shows a banner saying
so. Press stop and try again.

**Firmware updates halt the motion loop.** Writing to flash disables the instruction cache and
stalls both processor cores, so step pulses cannot be generated reliably during an update. The
controller therefore stops its motion loop for the duration of the write. The stepper drivers are
deliberately *not* disabled, so the axes keep their holding torque and a heavy cross slide will not
drift while the update runs.

**If an update is interrupted**, the controller sits showing `Updating firmware` and does nothing.
That is the intended outcome: the new image is written but not yet running, and the machine is
still rather than half-controlled. Power cycle it.

---

## Why the PIN is eight digits

Every setting on this controller is stored as a number, which is what lets the same table drive the
LCD menu, the serial interface and this page. The WiFi password has to live in that same storage,
so it is a number too.

WPA2 requires a password of at least 8 characters. If a shorter one is given, the ESP32 does not
report an error — it quietly brings up an **open** network that anyone can join. A PIN of `01234567`
stored as a number comes back as `1234567`, seven characters, and would do exactly that. So the
settings menu refuses anything that is not in the range 10000000–99999999, which guarantees eight
characters and no lost leading zero.

**Change it from the default.** Anyone who can join this network can reflash the controller.

---

## Cost of leaving it on

The WiFi driver runs at a much higher priority than the controller's own background tasks, so with
the radio active you may notice slightly less smooth manual jogging and a marginally laggier
display. The motion loop that generates step pulses runs on the other processor core and is not
affected, except during a firmware write.

If you are not using it, leave it off. Since it switches on and off from the panel without a
restart, turning it on only when you need it costs nothing.

---

## Changing the network name

`WIFI_SSID`, the channel and the default PIN are in `machine_config.h` alongside the rest of the
wiring-level configuration. The name is compile-time because, unlike the PIN, it is not something
the numeric settings table can hold.
