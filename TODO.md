# TODO

## Must do
* Some connections remain between copying (may be fixed)
* Some large loads destroy current board (e.g. DIS, DIS, ADD)
* Prevent double links
* Bump object connection limit to 16bit

## Should do
* Move code out of main() into satellite functions, inc. common logic
* Make all find_if use const&
* Clock correction when going from slow to fast (introduce 1s lag cap)
* Add Is- to all bools
* Stop off-screen draw
* Check that continuous-link doesn't add more than one link
* Event-driven redraw

## Could do
* Fast-mo
* Adaptive grid size, recreated on load and window resize
