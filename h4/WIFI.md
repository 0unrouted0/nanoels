# WiFi configuration and firmware updates

The controller can bring up its own WiFi network so you can edit every setting from a phone or
laptop and install new firmware without carrying a computer to the lathe and plugging in a cable.
It can also join a network you already have, so the machine is reachable from anywhere in the
workshop without leaving your phone cut off from the internet.

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
`NanoEls-H4`, and the **Enabled** item shows the address beside its value with the network it is on
underneath:

```
WiFi & updates 1/4
Enabled
Now yes   192.168.4.1
Own AP NanoEls-H4
```

Switching it back to `no` shuts the radio down again, also without a restart. Turning it on to
configure the machine and off again afterwards is a perfectly reasonable way to work.

If it shows `WiFi failed to start`, the PIN is the first thing to check — the setting switches
itself back off rather than retrying, so the controller is never stuck in a failing loop.

Changing the PIN while the network is up restarts it so the new password takes effect, which
disconnects anyone currently on it. That is the point of changing it.

The section holds four items: **Enabled**, **Access PIN**, **Join timeout** and **Forget network**.
The last two only matter once the machine has joined a network of yours, which is the next section.

## Connecting

Join the WiFi network **`NanoEls-H4`** using your PIN as the password, then browse to
**http://192.168.4.1** or **http://nanoels.local**.

Your phone will most likely warn that this network has no internet connection and offer to switch
back to mobile data. Tell it to stay connected, or the page will not load.

---

## Joining your own network

The machine can hang off your workshop router instead of making a network of its own. Your phone
then keeps its internet connection, and the page is reachable from anywhere the router reaches.

A network name cannot be typed on a numeric keypad, so this is done from the browser. Connect to
the machine's own access point first, as above.

1. Open the **WiFi & updates** section on the page.
2. Press **Scan**. The networks in range appear as buttons, strongest first, with a signal
   strength beside each name.
3. Tap the one you want, type its password, and press **Join**.

Joining takes a few seconds. On success the page tells you the machine's new address, and the
access point shuts down shortly afterwards — your phone drops off it, which is expected. Reconnect
your phone to the same network and carry on at **http://nanoels.local**, or at the IP address the
page gave you, which the panel also shows:

```
WiFi & updates 1/4
Enabled
Now yes   192.168.1.44
On Workshop
```

`nanoels.local` works out of the box on iPhones, Macs and Windows 10 or newer. Older Android
devices do not resolve it, so use the IP address there.

If the password was wrong or the router out of range, nothing is stored and nothing is lost: the
access point is never taken down during the attempt, so the page reports the refusal and you can
try again.

### At every restart afterwards

The sequence is the same every single time the controller starts:

1. If a network is stored, try to join it, for up to **Join timeout** seconds.
2. If that works, the machine is on your network and the panel shows its address there.
3. If it does not — router switched off, moved out of range, password changed — the machine brings
   up its own **`NanoEls-H4`** access point instead, exactly as it did before.

A failed attempt never deletes the stored network. Switch the router back on, restart the
controller, and it joins again. The only thing that forgets a network is the item that says so.

**Join timeout** is how long step 1 is allowed to take, from 5 to 120 seconds, 15 by default. It is
dead time at every start, so keep it short; raise it only if your router is slow to hand out
addresses.

### Forgetting it

On the panel, go to **Forget network** and press **play**. The item names the network it is about
to forget, which is the only confirmation it gets:

```
WiFi & updates 4/4
Forget network
Workshop
ON to forget
```

The stored name and password are erased and the radio comes back up as the machine's own access
point. The page has a **Forget** button that does the same thing.

---

## The PIN on somebody else's network

On the machine's own access point the WPA2 password is the gate: anyone who got onto the network
already proved they know the PIN, so the page asks for nothing.

On your workshop network there is no such gate — every device in the building can reach the page.
So the page asks for the same **Access PIN** before it will show or change anything:

```
Access PIN   [        ]  [ Unlock ]
```

Open without it: the version, the live status chips along the top, the running/stopped banner and
the derived-figures table. Watching what the machine is doing is not what the PIN is protecting.

Behind it: the settings list, saving any value, the settings backup, the stored G-code programs,
the network controls and firmware updates. The backup is in that list for a reason of its own — the
file it produces contains the access PIN as one of its lines.

Enter the PIN once per page load. Reload the page and you enter it again; nothing is remembered
between visits.

> **This is a lock, not encryption.** The PIN travels over plain HTTP, so it keeps the household
> and the rest of the shop floor out. It would not survive somebody capturing traffic on your
> network. If that is a concern, leave the machine on its own access point.

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
never disagree. The exceptions are the menu items that are not values at all but open a screen on
the controller — calibration, the thread database, **Forget network** — which have nothing to edit
over HTTP.

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

The file lists the access PIN among the settings, which is why it is one of the things the
[PIN prompt](#the-pin-on-somebody-elses-network) covers on a shared network.

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

**On a shared network, everyone can reach the page.** The machine's own access point keeps it to
whoever knows the PIN; a workshop router does not. That is what the
[PIN prompt](#the-pin-on-somebody-elses-network) is for, and why the firmware update is behind it.

**The router password is stored in plain text.** It lives in the ESP32's settings flash, which is
not encrypted. Anyone who walks off with the controller, or reads the chip out, can recover the
password of whatever network you joined. Put the machine on a guest network if that matters.

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

The same PIN does double duty: on a workshop network, where there is no WiFi password in front of
the machine, it is what unlocks the page.

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

## Compile-time settings

These live in `src/configs/machine_config_*.h` alongside the rest of the wiring-level
configuration. The network name is compile-time because, unlike the PIN, it is not something the
numeric settings table can hold.

| Constant | Default | What it is |
|---|---|---|
| `WIFI_SSID` | `NanoEls-H4` | Name of the machine's own access point. |
| `WIFI_CHANNEL` | `1` | Channel that access point uses. |
| `WIFI_ENABLED` | `false` | Whether the radio is on before anyone has touched the setting. |
| `WIFI_PIN_DEFAULT` | `13572468` | Starting access PIN. Change it. |
| `WIFI_HOSTNAME` | `nanoels` | Name announced over mDNS, so `nanoels.local` resolves. |
| `WIFI_AUTH_USER` | `nanoels` | User name the page sends with the PIN on a shared network. |
| `WIFI_STA_TIMEOUT_S` | `15` | Starting value for **Join timeout**, adjustable from the menu. |
