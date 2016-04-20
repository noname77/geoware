/*
 * Example Contiki test script (JavaScript).
 * A Contiki test script acts on mote output, such as via printf()'s.
 * The script may operate on the following variables:
 *  Mote mote, int id, String msg
 */

TIMEOUT(1000000, log.testOK()); /* milliseconds. print last msg at timeout */

var motes = sim.getMotes();
log.log("number of motes: " + motes.length + "\n");


// adapted from https://sourceforge.net/p/contiki/mailman/message/34696644/
// send positions from cooja to the contiki nodes
for (var i = 0; i < motes.length; i++) {
    WAIT_UNTIL(msg.startsWith('Waiting'));
    log.log("node id: " + id + "\n");
    x = mote.getInterfaces().getPosition().getXCoordinate();
    y = mote.getInterfaces().getPosition().getYCoordinate();
    log.log("x=" + x.toFixed(3) + " y=" + y.toFixed(3) + "\n");
    write(mote, ""+x);
    write(mote, ""+y);
    YIELD();
}

var txp = 7;
log.log("setting nodes tx power to: " + txp + "\n");
for (var i = 0; i < motes.length; i++) {
    write(motes[i], "txp "+txp);
}

YIELD();

// nodes fail
while(true) {
    WAIT_UNTIL(msg.startsWith('Waiting'));
    log.log("node id: " + id + "\n");
    x = mote.getInterfaces().getPosition().getXCoordinate();
    y = mote.getInterfaces().getPosition().getYCoordinate();
    log.log("x=" + x.toFixed(3) + " y=" + y.toFixed(3) + "\n");
    write(mote, ""+x);
    write(mote, ""+y);
    YIELD();
}