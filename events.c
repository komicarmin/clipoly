#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "_mingw.h"

double eps = 0.00000001;

typedef struct event event_t;

typedef struct point {
    double x;
    double y;
    struct point * prev;
    struct point * next;
} point_t;

typedef struct region {
    point_t *first;
    struct region *next;
} reg_t;

typedef struct segment {
    point_t * startp;
    point_t * endp;
    char myFillAbove;
    char myFillBelow;
    char otherFillAbove;
    char otherFillBelow;
    struct segment * next;
} seg_t;

typedef struct status {
    event_t *ev;
    struct status * prev;
    struct status * next;
} status_t;

typedef struct event {
    bool isStart;
    bool primary;
    point_t * pt;
    seg_t * seg;
    struct event *other;
    status_t *status;
    struct event * prev;
    struct event * next;
} event_t;

typedef struct intersection {
    point_t *pt;
    double alongA;
    double alongB;
} intersection_t;

// Global variable
status_t *status_root;

// Memory allocation with error handling
void *chkMalloc (size_t sz) {
    void *mem = malloc (sz);

    if (mem == NULL) {
        printf ("Out of memory! Exiting.\n");
        exit (1);
    }

    return mem;
}


#define MAXCN 50



// Print polygon points
void dumpPoly (point_t *first) {
    point_t * tmp = first;
    if (first == NULL) {
        printf ("  <<empty_polygon>>\n");
    } else {
        do {
            __mingw_printf("%.15f,%.15f\n", tmp->x, tmp->y);
            tmp = tmp->next;
        } while (tmp != first);
    }
}

void addPoint (point_t **first, double x, double y) {
    point_t *newest = chkMalloc (sizeof(*newest));
    newest->x = x;
    newest->y = y;
    newest->prev = NULL;
    newest->next = NULL;

    if(*first == NULL) {
        newest->prev = newest;
        newest->next = newest;
        *first = newest;
    }
    else {
        point_t *tmp = (*first)->prev;
        tmp->next = newest;
        newest->prev = tmp;
        newest->next = *first;
        (*first)->prev = newest;
    }
}

int *ReadFile(char *filename, reg_t **list_head) {
    double myvariable;

    FILE *myfile;
    myfile=fopen(filename, "r");
    if (myfile == NULL) {
        fprintf (stderr, "error: file open failed .\n");
        return 1;
    }

    char inpStr[MAXCN] = {0};
    int currChar = 0, prevChar = 0, i = 0, part = 0;
    double x = 0, y = 0, z = 0;
    bool added = false;

    // LIST OF POLYGONS
    reg_t *currList = *list_head;

    // POLYGON POINTS
    point_t *head = NULL;
    point_t* currPoly = head;

    while(currChar != EOF) {
        i = 0;
        while (i + 1 < MAXCN) {
            prevChar = currChar;
            currChar = fgetc(myfile);
            if(currChar == EOF || currChar == ' ' || currChar == ',') {
                break;
            }
            else if(currChar == '\n') {
                break;
            }
            else if(currChar == '*' && prevChar == '\n') {
                return NULL;
            }
            inpStr[i++] = currChar;
        }
        inpStr[i] = 0;   /* null-terminate */

        if((currChar == '\n' || currChar == EOF) && prevChar == '\n') {
            if(added)
                continue;
            reg_t *newListEl = malloc(sizeof(*newListEl));
            newListEl->first = head;
            newListEl->next = NULL;
            if(*list_head == NULL) {
                *list_head = newListEl;
                currList = *list_head;
            }
            else {
                currList->next = newListEl;
                currList = currList->next;
            }
            //printf("added poly\n");
            head = NULL;
            added = true;

            continue;
        }
        added = false;

        sscanf(inpStr, "%Lf", &myvariable);
        if(part == 0) {
            x = myvariable;
            //printf("x:%.15Lf ", myvariable);
            //__mingw_printf("x:%.15f ", myvariable);
            part++;
        }
        else if (part == 1) {
            y = myvariable;
            //__mingw_printf("y:%.15f ", myvariable);
            part++;
        }
        else if (part == 2) {
            z = myvariable;
            //__mingw_printf("z:%.15f ", myvariable);

            addPoint(&head, x, y);
            //printf("added point\n");

            x = 0, y = 0, z = 0, part = 0;
        }

    }
    fclose(myfile);

    return 0;
}

int pointsCompare(point_t **p1, point_t **p2){
    if (fabs((double)(*p1)->x - (*p2)->x) < eps) { // same vertical line?
        if (fabs((double)(*p1)->y - (*p2)->y) < eps) // same exact point?
            return 0;
        return (*p1)->y < (*p2)->y ? -1 : 1; // compare Y
    }
    return (*p1)->x < (*p2)->x ? -1 : 1; // compare X
}

bool pointAboveOrOnLine(point_t **pt, point_t **left, point_t **right){
    double Ax = (*left)->x;
    double Ay = (*left)->y;
    double Bx = (*right)->x;
    double By = (*right)->y;
    double Cx = (*pt)->x;
    double Cy = (*pt)->y;
    double res = (Bx - Ax) * (Cy - Ay) - (By - Ay) * (Cx - Ax);
    return  res >= -eps;
}

int eventCompare(bool p1_isStart, point_t **p1_1, point_t **p1_2, bool p2_isStart, point_t **p2_1, point_t **p2_2){

    int comp = pointsCompare(p1_1, p2_1);
    if (comp != 0)
        return comp;

    if (pointsCompare(p1_2, p2_2) == 0)
        return 0; // segments are equal

    if (p1_isStart != p2_isStart){
        return p1_isStart ? 1 : -1;
    }
    // manually calculate which one is below the other
    return pointAboveOrOnLine(p1_2, p2_isStart ? p2_1 : p2_2, p2_isStart ? p2_2 : p2_1) ? 1 : -1;
}

void eventAdd(event_t **ev_root, event_t **ev, point_t **other_pt){
    if(*ev_root == NULL) {
        *ev_root = *ev;
        return;
    }

    event_t *last = NULL;
    event_t *here = (*ev_root);
    while (here != NULL){
        if (eventCompare((*ev)->isStart, &(*ev)->pt, other_pt, here->isStart, &here->pt, &here->other->pt) < 0){
            (*ev)->prev = here->prev;
            (*ev)->next = here;
            if(here != *ev_root)
                (*ev)->prev->next = *ev;
            else
                *ev_root = *ev;
            (*ev)->next->prev = *ev;
            return;
        }
        last = here;
        here = here->next;
    }
    last->next = (*ev);
    (*ev)->prev = last;
    (*ev)->next = NULL;
}

event_t *eventAddSegmentStart(event_t **ev_root, seg_t **seg, bool primary){
    event_t *ev_start = chkMalloc (sizeof(*ev_start));
    ev_start->isStart = true;
    ev_start->primary = primary;
    ev_start->pt = (*seg)->startp;
    ev_start->seg = (*seg);
    ev_start->other = NULL;
    ev_start->status = NULL;
    ev_start->next = NULL;
    ev_start->prev = NULL;

    eventAdd(ev_root, &ev_start, &((*seg)->endp));
    return ev_start;
}

event_t *eventAddSegmentEnd(event_t **ev_root, event_t **ev_start, seg_t **seg, bool primary){
    event_t *ev_end = chkMalloc (sizeof(*ev_end));
    ev_end->isStart = false;
    ev_end->primary = primary;
    ev_end->pt = (*seg)->endp;
    ev_end->seg = (*seg);
    ev_end->other = (*ev_start);
    ev_end->status = NULL;
    ev_end->next = NULL;
    ev_end->prev = NULL;

    (*ev_start)->other = ev_end;
    eventAdd(ev_root, &ev_end, &(*ev_start)->pt);
    return ev_end;
}

event_t *eventAddSegment(event_t **ev_root, seg_t **seg, bool primary){
    event_t *ev_start = eventAddSegmentStart(ev_root, seg, primary);
    eventAddSegmentEnd(ev_root, &ev_start, seg,primary);
    return ev_start;
}

void dumpEventList(event_t *first) {
    event_t * tmp = first;
    int c = 1;
    if (first == NULL) {
        printf ("  <<empty_eventList>>\n");
    } else {
        do {
            printf ("%d - X:%.1lf Y:%.1lf, %s (OTHER: X:%.1lf Y:%.1lf)\n",c++, tmp->pt->x, tmp->pt->y, tmp->isStart ? "Start" : "End", tmp->other->pt->x, tmp->other->pt->y);
            tmp = tmp->next;
        } while (tmp != NULL);
    }
}

event_t *polyToEvents(event_t **ev_root, point_t **poly, bool primary) {
    point_t * tmp = *poly;
    if (poly != NULL) {
        do {
            int forward = pointsCompare(&(tmp->prev), &tmp);
            if (forward == 0) continue;

            seg_t *s = malloc(sizeof(*s));
            s->myFillAbove = -1;
            s->myFillBelow = -1;
            s->otherFillAbove = -1;
            s->otherFillBelow = -1;
            s->next = NULL;

            if (forward > 0) {
                s->startp = tmp;
                s->endp = tmp->prev;
            }
            else {
                s->startp = tmp->prev;
                s->endp = tmp;
            }

            eventAddSegment(ev_root, &s, primary);
            tmp = tmp->next;
        } while (tmp != *poly);
    }
}

void segmentsToEvents(event_t **ev_root, seg_t *poly, bool primary) {
    seg_t *tmp = poly;
    while(tmp != NULL) {
        seg_t *s = malloc(sizeof(*s));
        s->startp = tmp->startp;
        s->endp = tmp->endp;
        s->myFillAbove = tmp->myFillAbove;
        s->myFillBelow = tmp->myFillBelow;
        s->otherFillAbove = -1;
        s->otherFillBelow = -1;
        s->next = NULL;

        eventAddSegment(ev_root, &s, primary);
        tmp = tmp->next;
    }

}

event_t *deleteFirstEvent(event_t **ev_root)
{
    if(*ev_root != NULL) {
        event_t *temp = *ev_root;
        temp = temp->next;
        *ev_root = temp;
    }
    return *ev_root;
}

bool pointsCollinear(point_t *pt1, point_t *pt2, point_t *pt3){
    double dx1 = pt1->x - pt2->x;
    double dy1 = pt1->y - pt2->y;
    double dx2 = pt2->x - pt3->x;
    double dy2 = pt2->y - pt3->y;
    double res = dx1 * dy2 - dx2 * dy1;
    res = res < 0 ? -1*res : res;
    return res < eps;
}

int statusCompare(event_t **ev1, event_t **ev2){point_t *a1 = (*ev1)->seg->startp;
    point_t *a2 = (*ev1)->seg->endp;
    point_t *b1 = (*ev2)->seg->startp;
    point_t *b2 = (*ev2)->seg->endp;

    if (pointsCollinear(a1, b1, b2)){
        if (pointsCollinear(a2, b1, b2))
            return 1;
        return pointAboveOrOnLine(&a2, &b1, &b2) ? 1 : -1;
    }
    return pointAboveOrOnLine(&a1, &b1, &b2) ? 1 : -1;
}

status_t *statusFindTransition(event_t **ev, status_t **before, status_t **after) {
    status_t *last = NULL;
    status_t *here = status_root;
    while (here != NULL){
        int c = statusCompare(ev, &here->ev);
        if (c > 0)
            break;
        last = here;
        here = here->next;
    }
    if(last != NULL)
        *before = last;
    *after = here;

    return here;
}

intersection_t *linesIntersect (point_t **a0, point_t **a1, point_t **b0, point_t **b1) {
    double adx = (double)(*a1)->x - (double)(*a0)->x;
    double ady = (double)(*a1)->y - (double)(*a0)->y;
    double bdx = (double)(*b1)->x - (double)(*b0)->x;
    double bdy = (double)(*b1)->y - (double)(*b0)->y;

    double axb = adx * bdy - ady * bdx;

    if (fabs(axb) < eps)
        return NULL;

    double dx = (*a0)->x - (*b0)->x;
    double dy = (*a0)->y - (*b0)->y;

    double A = (bdx * dy - bdy * dx) / axb;
    double B = (adx * dy - ady * dx) / axb;

    intersection_t *res = chkMalloc(sizeof(intersection_t));

    point_t *pt = malloc(sizeof(*pt));
    pt->x = (*a0)->x + A * adx;
    pt->y = (*a0)->y + A * ady;
    res->pt = pt;

    if (A <= -eps)
        res->alongA = -2;
    else if (A < eps)
        res->alongA = -1;
    else if (A - 1 <= -eps)
        res->alongA = 0;
    else if (A - 1 < eps)
        res->alongA = 1;
    else
        res->alongA = 2;

    if (B <= -eps)
        res->alongB = -2;
    else if (B < eps)
        res->alongB = -1;
    else if (B - 1 <= -eps)
        res->alongB = 0;
    else if (B - 1 < eps)
        res->alongB = 1;
    else
        res->alongB = 2;

    return res;
}

void removeStatus(status_t *del) {
    if (status_root == NULL || del == NULL)
        return;

    if (status_root == del)
        status_root = del->next;

    if (del->next != NULL)
        del->next->prev = del->prev;

    if (del->prev != NULL)
        del->prev->next = del->next;

    free(del);
}

void removeEvent(event_t **event_root, event_t *del) {
    if (*event_root == NULL || del == NULL)
        return;

    if (*event_root == del)
        *event_root = del->next;

    if (del->next != NULL)
        del->next->prev = del->prev;

    if (del->prev != NULL)
        del->prev->next = del->next;
}

void eventUpdateEnd(event_t **ev_root, event_t **ev, point_t **end){
    removeEvent(ev_root, (*ev)->other);
    (*ev)->seg->endp = *end;
    (*ev)->other->pt = *end;
    eventAdd(ev_root, &(*ev)->other, &(*ev)->pt);
}

event_t *eventDivide(event_t **ev_root, event_t **ev, point_t **pt){
    seg_t *ns = malloc(sizeof(*ns));
    ns->startp = *pt;
    ns->myFillAbove = (*ev)->seg->myFillAbove;
    ns->myFillBelow = (*ev)->seg->myFillBelow;
    ns->otherFillAbove = -1;
    ns->otherFillBelow = -1;
    ns->endp = (*ev)->seg->endp;
    ns->next = NULL;

    eventUpdateEnd(ev_root, ev, pt);
    return eventAddSegment(ev_root, &ns, (*ev)->primary);
}

bool pointsSame(point_t *p1, point_t *p2) {
    if(p1->x == p2->x && p1->y == p2->y)
        return true;
    return false;
}

bool pointBetween(point_t *p, point_t *left, point_t *right){
    double d_py_ly = p->y - left->y;
    double d_rx_lx = right->x - left->x;
    double d_px_lx = p->x - left->x;
    double d_ry_ly = right->y - left->y;

    double dot = d_px_lx * d_rx_lx + d_py_ly * d_ry_ly;
    if (dot < eps)
        return false;

    double sqlen = d_rx_lx * d_rx_lx + d_ry_ly * d_ry_ly;
    if (dot - sqlen > -eps)
        return false;

    return true;
}

event_t *checkIntersection(event_t **ev_root, event_t **ev1, event_t **ev2) {
    seg_t *seg1 = (*ev1)->seg;
    seg_t *seg2 = (*ev2)->seg;
    point_t *a1 = seg1->startp;
    point_t *a2 = seg1->endp;
    point_t *b1 = seg2->startp;
    point_t *b2 = seg2->endp;

    intersection_t *i = linesIntersect(&a1, &a2, &b1, &b2);

    if(i == NULL) {
        // segments overlap or are parallel
        if (!pointsCollinear(a1, a2, b1))
            return NULL;

        // segments touch at endpoint = no intersection
        if (pointsSame(a1, b2) || pointsSame(a2, b1))
            return NULL;

        bool a1_equ_b1 = pointsSame(a1, b1);
        bool a2_equ_b2 = pointsSame(a2, b2);

        // segments are equal?
        if (a1_equ_b1 && a2_equ_b2)
            return *ev2;

        bool a1_between = !a1_equ_b1 && pointBetween(a1, b1, b2);
        bool a2_between = !a2_equ_b2 && pointBetween(a2, b1, b2);

        if (a1_equ_b1){
            if (a2_between){
                //  (a1)---(a2)
                //  (b1)----------(b2)
                eventDivide(ev_root, ev2, &a2);
            }
            else{
                //  (a1)----------(a2)
                //  (b1)---(b2)
                eventDivide(ev_root, ev1, &b2);
            }
            return *ev2;
        }
        else if (a1_between){
            if (!a2_equ_b2){
                if (a2_between){
                    //         (a1)---(a2)
                    //  (b1)-----------------(b2)
                    eventDivide(ev_root, ev2, &a2);
                }
                else{
                    //         (a1)----------(a2)
                    //  (b1)----------(b2)
                    eventDivide(ev_root, ev1, &b2);
                }
            }

            //         (a1)---(a2)
            //  (b1)----------(b2)
            eventDivide(ev_root, ev2, &a1);
        }
    }
    else {
        // lines intersect

        // is A divided between its endpoints?
        if (i->alongA == 0){
            if (i->alongB == -1) // at b1
                eventDivide(ev_root, ev1, &b1);
            else if (i->alongB == 0) //between B's endpoints
                eventDivide(ev_root, ev1, &(i->pt));
            else if (i->alongB == 1) // at b2
                eventDivide(ev_root, ev1, &b2);
        }

        // is B divided between its endpoints?
        if (i->alongB == 0){
            if (i->alongA == -1) // at a1
                eventDivide(ev_root, ev2, &a1);
            else if (i->alongA == 0) // between A's endpoints
                eventDivide(ev_root, ev2, &(i->pt));
            else if (i->alongA == 1) // at a2
                eventDivide(ev_root, ev2, &a2);
        }
    }
    return NULL;
}

event_t *checkBothIntersections(event_t **ev_root, event_t **ev, event_t **above, event_t **below) {
    if (*above != NULL){
        event_t *eve = checkIntersection(ev_root, ev, above);
        if (eve != NULL)
            return eve;
    }
    if (*below != NULL)
        return checkIntersection(ev_root, ev, below);
    return NULL;
}

void addStatusAfter(status_t **before, status_t **after, status_t **new) {
    if(*before != NULL) {
        (*before)->next = *new;
        (*new)->prev = *before;
    }
    else {
        status_root = *new;
    }

    if(*after != NULL) {
        (*after)->prev = *new;
        (*new)->next = *after;
    }
    else {
        (*new)->next = NULL;
    }
}

void addSegment(seg_t **seg_root, seg_t **new) {
    if(*seg_root == NULL) {
        *seg_root = *new;
    }
    else {
        seg_t *tmp = *seg_root;
        while(tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = *new;
    }
}

void dumpStatus() {
    status_t* tmp = status_root;

    printf("DUMP--------START\n");
    while(tmp != NULL) {
        printf("%.0lf,%.0lf - OTHER: %.0lf,%.0lf\n", tmp->ev->pt->x, tmp->ev->pt->y, tmp->ev->other->pt->x, tmp->ev->other->pt->y);
        tmp = tmp->next;
    }
    printf("DUMP--------END\n");
}

seg_t *eventLoop(event_t **ev_root, bool selfIntersection, bool primaryPolyInverted, bool secondaryPolyInverted) {
    seg_t *segments = malloc(sizeof(*segments));
    segments = NULL;

    status_root = NULL;

    while (*ev_root != NULL) {
        event_t *ev = *ev_root;
        if (ev->isStart) {
            // START event

            status_t *before = NULL;
            status_t *after = NULL;
            status_t *transition = statusFindTransition(&ev, &before, &after);

            // find events that are above and below
            event_t *above = (before != NULL) ? before->ev : NULL;
            event_t *below = (after != NULL) ? after->ev : NULL;

            // check for intersections
            event_t *eve = chkMalloc(sizeof(*eve));
            eve = NULL;
            eve = checkBothIntersections(ev_root, &ev, &above, &below);

            if (eve != NULL){
                // ev and eve are equal

                // update eve fill annotations based on ev
                if (selfIntersection == true){
                    bool toggle;
                    if (ev->seg->myFillBelow == -1)
                        toggle = true;
                    else
                        toggle = ev->seg->myFillAbove != ev->seg->myFillBelow;

                    // merge two segments that belong to the same polygon
                    if (toggle)
                        eve->seg->myFillAbove = 1 - eve->seg->myFillAbove;
                }
                else{
                    // merge two segments that belong to different polygons

                    eve->seg->otherFillBelow = (char)ev->seg->myFillBelow;
                    eve->seg->otherFillAbove = (char)ev->seg->myFillAbove;
                }

                removeEvent(ev_root, ev->other);
                removeEvent(ev_root, ev);
            }

            if (*ev_root != ev){
                // something was inserted before us, so loop back and process it
                continue;
            }

            // calculate fill flags
            if(selfIntersection == true) {
                bool toggle;
                if (ev->seg->myFillBelow == -1) // new segment, no fill flags yet
                    toggle = true;
                else
                    toggle = ev->seg->myFillAbove != ev->seg->myFillBelow;

                if (below == NULL) {
                    ev->seg->myFillBelow = primaryPolyInverted == true ? 1 : 0;
                } else {
                    ev->seg->myFillBelow = below->seg->myFillAbove;
                }

                if (toggle == true)
                    ev->seg->myFillAbove = 1 - ev->seg->myFillBelow;
                else
                    ev->seg->myFillAbove = ev->seg->myFillBelow;
            }
            else {
                // are we inside the other polygon
                bool inside;
                if (below == NULL){
                    if (ev->primary == true)
                        inside = secondaryPolyInverted;
                    else
                        inside = primaryPolyInverted;
                }
                else{
                    if (ev->primary == below->primary)
                        inside = below->seg->otherFillAbove == 1;
                    else
                        inside = below->seg->myFillAbove == 1;
                }
                ev->seg->otherFillAbove = inside;
                ev->seg->otherFillBelow = inside;
            }

            // create the status node
            status_t *st = malloc(sizeof(*st));
            st->ev = ev;
            st->next = NULL;
            st->prev = NULL;

            // insert at transition location
            addStatusAfter(&before, &after, &st);
            ev->other->status = st;
        }
        else {
            // END event
            status_t *st = ev->status;

            if (st->prev != NULL && st->next != NULL){
                checkIntersection(ev_root, &(st->prev->ev), &(st->next->ev));
            }

            // remove status
            removeStatus(st);

            if (ev->primary == false){
                // make sure myFill  points to primary polygon
                char tmp = ev->seg->myFillBelow;
                ev->seg->myFillBelow = ev->seg->otherFillBelow;
                ev->seg->otherFillBelow = tmp;

                tmp = ev->seg->myFillAbove;
                ev->seg->myFillAbove = ev->seg->otherFillAbove;
                ev->seg->otherFillAbove = tmp;
            }

            addSegment(&segments, &ev->seg);
        }
        deleteFirstEvent(ev_root);
    }

    return segments;
}

seg_t *select(seg_t *segments, int opID) {
    int selection[16];
    int unionOp[16] = {0, 2, 1, 0, 2, 2, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0};
    int intersectionOp[16] = {0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 1, 1, 0, 2, 1, 0};
    int differenceOp[16] = {0, 0, 0, 0, 2, 0, 2, 0, 1, 1, 0, 0, 0, 1, 2, 0};
    int differenceRevOp[16] = {0, 2, 1, 0, 0, 0, 1, 1, 0, 2, 0, 2, 0, 0, 0, 0};
    int xorOp[16] = {0, 2, 1, 0, 2, 0, 0, 1, 1, 0, 0, 2, 0, 1, 2, 0};

    if (opID == 1) memmove(selection, unionOp, sizeof(selection));
    if (opID == 2) memmove(selection, intersectionOp, sizeof(selection));
    if (opID == 3) memmove(selection, differenceOp, sizeof(selection));
    if (opID == 4) memmove(selection, differenceRevOp, sizeof(selection));
    if (opID == 5) memmove(selection, xorOp, sizeof(selection));

    seg_t *result = NULL;

    while(segments != NULL) {
        int index = (segments->myFillAbove == 1 ? 8 : 0) +
                    (segments->myFillBelow == 1 ? 4 : 0) +
                    (segments->otherFillAbove == 1 ? 2 : 0) +
                    (segments->otherFillBelow == 1 ? 1 : 0);
        if (selection[index] != 0) {
            seg_t *ns = malloc(sizeof(*ns));
            ns->startp = segments->startp;
            ns->endp = segments->endp;
            ns->myFillAbove = selection[index] == 1 ? 1 : 0;
            ns->myFillBelow = selection[index] == 2 ? 1 : 0;
            ns->otherFillAbove = -1;
            ns->otherFillBelow = -1;
            ns->next = NULL;

            addSegment(&result, &ns);
        }
        segments = segments->next;
    }

    return result;
}

void reverse(point_t **head_ref)
{
    point_t *temp = NULL;
    point_t *current = *head_ref;

    do {
        temp = current->prev;
        current->prev = current->next;
        current->next = temp;
        current = current->prev;
    } while (current != *head_ref);

    if(temp != NULL )
        *head_ref = temp->prev;
}

void addReg(reg_t **region, reg_t *r) {
    reg_t *tmp = *region;
    if(*region == NULL) {
        *region = r;
        return;
    }

    while(tmp->next != NULL) {
        tmp = tmp->next;
    }

    tmp->next = r;
}

void removeReg(reg_t **region, reg_t *r) {
    reg_t *curr = *region;
    reg_t *prev = NULL;

    while(curr != NULL) {
        if(curr == r) {
            if(prev == NULL) {
                *region = curr->next;
            }
            else {
                prev->next = curr->next;
            }
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

reg_t *merge(seg_t *segments) {
    reg_t *regions = NULL;
    reg_t *res = NULL;

    while(segments != NULL) {
        reg_t *ch = malloc(sizeof(*ch));
        ch->first = NULL;
        ch->next = NULL;
        addPoint(&ch->first, segments->startp->x, segments->startp->y);
        addPoint(&ch->first, segments->endp->x, segments->endp->y);

        addReg(&regions, ch);

        segments = segments->next;
    }

    // Loop until there is only one
    reg_t *tmpChain = regions;
    while(true) {
        if(regions != NULL)
            tmpChain = regions;
        else
            break;

        reg_t *tmpChainSecPrev = tmpChain;
        reg_t *tmpChainSec = tmpChain->next;
        while(tmpChainSec != NULL) {
            if(pointsSame(tmpChain->first->prev, tmpChainSec->first)) {
                // Chain Tail = Segment Head
                // CH----------CT=SH----------ST

                if(pointsCollinear(tmpChain->first->prev->prev, tmpChain->first->prev, tmpChainSec->first->next) == true) {
                    // We don't need the middle point, because it lays on the line between 2 edges
                    tmpChain->first->prev->prev->next = tmpChainSec->first->next;
                    tmpChainSec->first->next->prev = tmpChain->first->prev->prev;
                }
                else {
                    tmpChain->first->prev->next = tmpChainSec->first->next;
                    tmpChainSec->first->next->prev = tmpChain->first->prev;
                }
                tmpChainSec->first->prev->next = tmpChain->first;
                tmpChain->first->prev = tmpChainSec->first->prev;

                if(pointsSame(tmpChain->first, tmpChain->first->prev)) {
                    // We have a closed chain
                    // Get rid of last point because it is same as first
                    tmpChain->first->prev->prev->next = tmpChain->first;
                    tmpChain->first->prev = tmpChain->first->prev->prev;

                    if(pointsCollinear(tmpChain->first->prev, tmpChain->first, tmpChain->first->next) == true) {
                        tmpChain->first->prev->next = tmpChain->first->next;
                        tmpChain->first->next->prev = tmpChain->first->prev;
                        tmpChain->first = tmpChain->first->next;
                    }

                    reg_t *newReg = malloc(sizeof(*newReg));
                    newReg->first = tmpChain->first;
                    newReg->next = NULL;
                    addReg(&res, newReg);
                    removeReg(&regions, tmpChain);
                }

                removeReg(&regions, tmpChainSec);
                break;
            }
            else if(pointsSame(tmpChain->first, tmpChainSec->first->prev)) {
                // Chain Head = Segment Tail
                // SH----------ST=CH----------CT
                if (pointsCollinear(tmpChainSec->first->prev->prev, tmpChainSec->first->prev, tmpChain->first->next) ==
                    true) {
                    // We don't need the middle point, because it lays on the line between 2 edges
                    tmpChainSec->first->prev->prev->next = tmpChain->first->next;
                    tmpChain->first->next->prev = tmpChainSec->first->prev->prev;
                } else {
                    tmpChainSec->first->prev->next = tmpChain->first->next;
                    tmpChain->first->next->prev = tmpChainSec->first->prev;
                }
                tmpChain->first->prev->next = tmpChainSec->first;
                tmpChainSec->first->prev = tmpChain->first->prev;

                if (pointsSame(tmpChainSec->first, tmpChainSec->first->prev)) {
                    // We have a closed chain
                    // Get rid of last point because it is same as first
                    tmpChainSec->first->prev->prev->next = tmpChainSec->first;
                    tmpChainSec->first->prev = tmpChainSec->first->prev->prev;

                    if (pointsCollinear(tmpChainSec->first->prev, tmpChainSec->first, tmpChainSec->first->next) ==
                        true) {
                        tmpChainSec->first->prev->next = tmpChainSec->first->next;
                        tmpChainSec->first->next->prev = tmpChainSec->first->prev;
                        tmpChainSec->first = tmpChainSec->first->next;
                    }

                    reg_t *newReg = malloc(sizeof(*newReg));
                    newReg->first = tmpChainSec->first;
                    newReg->next = NULL;
                    addReg(&res, newReg);
                    removeReg(&regions, tmpChainSec);
                }

                removeReg(&regions, tmpChain);
                break;
            }
            else if(pointsSame(tmpChain->first, tmpChainSec->first)) {
                // Chain Head = Segment Head
                // CT----------CH=SH----------CT
                reverse(&tmpChainSec->first);

                if (pointsCollinear(tmpChainSec->first->prev->prev, tmpChainSec->first->prev, tmpChain->first->next) ==
                    true) {
                    // We don't need the middle point, because it lays on the line between 2 edges
                    tmpChainSec->first->prev->prev->next = tmpChain->first->next;
                    tmpChain->first->next->prev = tmpChainSec->first->prev->prev;
                } else {
                    tmpChainSec->first->prev->next = tmpChain->first->next;
                    tmpChain->first->next->prev = tmpChainSec->first->prev;
                }
                tmpChain->first->prev->next = tmpChainSec->first;
                tmpChainSec->first->prev = tmpChain->first->prev;

                if (pointsSame(tmpChainSec->first, tmpChainSec->first->prev)) {
                    // We have a closed chain
                    // Get rid of last point because it is same as first
                    tmpChainSec->first->prev->prev->next = tmpChainSec->first;
                    tmpChainSec->first->prev = tmpChainSec->first->prev->prev;

                    if (pointsCollinear(tmpChainSec->first->prev, tmpChainSec->first, tmpChainSec->first->next) ==
                        true) {
                        tmpChainSec->first->prev->next = tmpChainSec->first->next;
                        tmpChainSec->first->next->prev = tmpChainSec->first->prev;
                        tmpChainSec->first = tmpChainSec->first->next;
                    }

                    reg_t *newReg = malloc(sizeof(*newReg));
                    newReg->first = tmpChainSec->first;
                    newReg->next = NULL;
                    addReg(&res, newReg);
                    removeReg(&regions, tmpChainSec);
                }

                removeReg(&regions, tmpChain);
                break;
            }
            else if(pointsSame(tmpChain->first->prev, tmpChainSec->first->prev)) {
                // Chain Tail = Segment Tail
                // CH----------CT=ST----------SH
                reverse(&tmpChainSec->first);

                if(pointsCollinear(tmpChain->first->prev->prev, tmpChain->first->prev, tmpChainSec->first->next) == true) {
                    // We don't need the middle point, because it lays on the line between 2 edges
                    tmpChain->first->prev->prev->next = tmpChainSec->first->next;
                    tmpChainSec->first->next->prev = tmpChain->first->prev->prev;
                }
                else {
                    tmpChain->first->prev->next = tmpChainSec->first->next;
                    tmpChainSec->first->next->prev = tmpChain->first->prev;
                }
                tmpChainSec->first->prev->next = tmpChain->first;
                tmpChain->first->prev = tmpChainSec->first->prev;

                if(pointsSame(tmpChain->first, tmpChain->first->prev)) {
                    // We have a closed chain
                    // Get rid of last point because it is same as first
                    tmpChain->first->prev->prev->next = tmpChain->first;
                    tmpChain->first->prev = tmpChain->first->prev->prev;

                    if(pointsCollinear(tmpChain->first->prev, tmpChain->first, tmpChain->first->next) == true) {
                        tmpChain->first->prev->next = tmpChain->first->next;
                        tmpChain->first->next->prev = tmpChain->first->prev;
                        tmpChain->first = tmpChain->first->next;
                    }

                    reg_t *newReg = malloc(sizeof(*newReg));
                    newReg->first = tmpChain->first;
                    newReg->next = NULL;
                    addReg(&res, newReg);
                    removeReg(&regions, tmpChain);
                }

                removeReg(&regions, tmpChainSec);
                break;
            }

            tmpChainSecPrev = tmpChainSecPrev->next;
            tmpChainSec = tmpChainSec->next;
        }
    }
    return res;
}