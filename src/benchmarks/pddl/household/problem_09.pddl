(define (problem pddl)
	(:domain household)
	(:objects
	c0 c1 c2 c3 c4 c5 c6 c7 c8 - cup
	dw - dishwasher
	dining_table - table
	cup_shelf - shelf
	s1 s2 s3 s4 s5 s6 s7 s8 s9 s10 - spot
	dw_front dw_side dining_room - location
	)


	(:init
		(reachable s1 dw_front)
		(reachable s2 dw_front)
		(reachable s3 dw_front)
		(reachable s4 dw_side)
		(reachable s5 dw_side)
		(reachable s6 dw_front)
		(reachable s7 dw_front)
		(reachable s8 dw_front)
		(reachable s9 dw_side)
		(reachable s10 dw_side)
		(clean c0)
		(clean c2)
		(clean c4)
		(clean c6)
		(clean c8)
		(robot_at dining_room)
		(alignable dw_front dw)
		(alignable dw_side dw)
		(alignable dining_table dining_table)
		(alignable cup_shelf cup_shelf)
		(at c0 dining_table)
		(at c1 dining_table)
		(at c2 dining_table)
		(at c3 dining_table)
		(at c4 dining_table)
		(at c5 dining_table)
		(at c6 dining_table)
		(at c7 dining_table)
		(at c8 dining_table)
	)
	(:goal (and
		(not (exists (?c - cup) (holding ?c)))
		(forall (?c - cup) (and (kif_clean ?c)
		                        (or (clean ?c) (at ?c dw))
		                        (or (not (clean ?c)) (at ?c cup_shelf))
		                   )
		)
 		)
	)
	(:metric minimize (total-time))
)
