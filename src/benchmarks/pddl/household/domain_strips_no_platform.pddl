(define (domain household)
	(:requirements :adl :equality :typing :durative-actions)
	(:types
		 location cup spot - object
		 dishwasher shelf table - location
	)
	(:predicates
		(robot_busy)
		(at ?param1 - cup ?param2 - location)
		(clean ?param1 - cup)
		(holding ?param1 - cup)
		(kif_clean ?param1 - cup)
		(occupied ?param1 - spot)
		(reachable ?param1 - spot ?param2 - location)
		(robot_at ?param1 - location)
	)

	(:durative-action goto
		:parameters (?source - location ?target - location )
		:duration (= ?duration 5)
		:condition
			( and
				; prevents to simultaneously drive to multiple locations
				(at start (not (robot_busy)))
				(over all (robot_busy))
				(at start ( not ( robot_at ?target ) ))
				(at start ( robot_at ?source ) )
				; it must be known whether a cup is dirty or not, before it can be moved
				(at start (forall (?c - cup) (or (not (holding ?c)) (kif_clean ?c))))
			)
		:effect ( and
				(at start (robot_busy))
				(at end (not (robot_busy)))
				(at end ( robot_at ?target ))
				(at end (not ( robot_at ?source)))
			)
	)
	(:durative-action pick_up
		:parameters ( ?cup - cup ?pos - location)
		:duration (= ?duration 10)
		:condition
			( and
				; prevents to simultaneously pick multiple cups
				(at start (not (robot_busy)))
				(over all (robot_busy))
				(at start ( robot_at ?pos ) )
				(over all ( robot_at ?pos ) )
				(at start ( not ( exists (?c - cup) ( holding ?c )) ))
				(over all ( not ( exists (?c - cup) ( holding ?c )) ))
				(at start ( at ?cup ?pos ))
				(over all ( at ?cup ?pos ))
			)
		:effect ( and
				(at start (robot_busy))
				(at end (not (robot_busy)))
				(at end ( holding ?cup ))
				(at end (not ( at ?cup ?l_3 ) ))
			)
	)
	(:durative-action put_in_dishwasher
		:parameters ( ?cup - cup ?spot - spot ?dw - dishwasher )
		:duration (= ?duration 10)
		:condition
			( and
				(at start ( robot_at ?dw ) )
				(over all ( robot_at ?dw ) )
				(at start ( holding ?cup ))
				(at start ( not ( occupied ?spot ) ))
			)
		:effect ( and
				(at end ( at ?cup ?dw ))
				(at end (occupied ?spot ))
				(at end ( not ( holding ?cup ) ))
			)
	)
	(:durative-action put_down
		:parameters ( ?cup - cup ?shelf - shelf )
		:duration (= ?duration 15)
		:condition
			( and
				(at start ( holding ?cup ))
				(over all ( holding ?cup ))
				(at start ( robot_at ?shelf ))
				(over all ( robot_at ?shelf ))
			)
		:effect ( and
				(at end ( at ?cup ?shelf ))
				(at end ( not ( holding ?cup ) ))
			)
	)
	(:durative-action is_cup_clean
		:parameters ( ?aparam1 - cup )
		:duration (= ?duration 2)
		:condition
			( and
				(at start ( holding ?aparam1 ))
				(over all ( holding ?aparam1 ))
			)
		:effect ( and
				(at end ( kif_clean ?aparam1 ))
			)
	)
)

