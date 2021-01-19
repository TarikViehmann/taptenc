(define (domain household)
	(:requirements :adl :equality :typing :durative-actions)
	(:types
		 location cup spot - object
		 dishwasher shelf table - location
	)
	(:predicates
		(alignable ?param1 - location ?param2 - location)
		(aligned ?param1 - location)
		(robot_busy)
		(at ?param1 - cup ?param2 - location)
		(clean ?param1 - cup)
		(holding ?param1 - cup)
		(kif_clean ?param1 - cup)
		(looking_at ?param1 - location)
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
				(at start ( not ( exists (?l - location) ( or ( aligned ?l ) ( looking_at ?l ) )) ))
				(over all ( not ( exists (?l - location) ( or ( aligned ?l ) ( looking_at ?l ) )) ))
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
	(:durative-action back_off
		:parameters ( ?loc - location )
		:duration (= ?duration 2)
		:condition
			( and
				(at start ( aligned ?loc ))
				(over all ( aligned ?loc ))
				(at start ( not ( looking_at ?loc ) ))
			)
		:effect ( and
				(at end ( not ( aligned ?loc ) ))
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
				(at start ( not ( exists (?c - cup) ( holding ?c )) ))
				(over all ( not ( exists (?c - cup) ( holding ?c )) ))
				(at start ( robot_at ?pos ) )
				(over all ( robot_at ?pos ) )
				(at start ( looking_at ?pos ))
				(at start ( aligned ?pos ))
				(at start ( at ?cup ?pos ))
				(over all ( looking_at ?pos ))
				(over all ( aligned ?pos ))
				(over all ( at ?cup ?pos ))
			)
		:effect ( and
				(at start (robot_busy))
				(at end (not (robot_busy)))
				(at end ( holding ?cup ))
				(at end (not ( at ?cup ?l_3 ) ))
			)
	)
	(:durative-action look_at
		:parameters ( ?loc - location )
		:duration (= ?duration 2)
		:condition
			( and
				(at start ( aligned ?loc ))
				(over all ( aligned ?loc ))
			)
		:effect ( and
				(at end ( looking_at ?loc ))
			)
	)
	(:durative-action look_up
		:parameters ( ?loc - location )
		:duration (= ?duration 2)
		:condition
			( and
				(at start ( looking_at ?loc ))
			)
		:effect ( and
				(at end ( not ( looking_at ?loc ) ))
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
				(at start ( exists (?l - location) ( and ( aligned ?l ) ( reachable ?spot ?l ) )))
				(over all ( exists (?l - location) ( and ( aligned ?l ) ( reachable ?spot ?l ) )))
			)
		:effect ( and
				(at end ( at ?cup ?dw ))
				(at end (occupied ?spot ))
				(at end ( not ( holding ?cup ) ))
			)
	)
	(:durative-action align_to
		:parameters ( ?loc - location )
		:duration (= ?duration 2)
		:condition
			( and
				(at start ( exists (?fromLoc - location) ( and ( robot_at ?fromLoc ) ( alignable ?loc ?fromLoc ) )))
			)
		:effect ( and
				(at end ( aligned ?loc ))
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
				(at start ( looking_at ?shelf ))
				(over all ( looking_at ?shelf ))
			)
		:effect ( and
				(at end ( at ?cup ?shelf ))
				(at end ( not ( holding ?cup ) ))
			)
	)
	(:durative-action is_cup_clean
		:parameters ( ?cup - cup )
		:duration (= ?duration 2)
		:condition
			( and
				(at start ( holding ?cup ))
				(over all ( holding ?cup ))
				(at start ( exists (?l - location) ( looking_at ?l )))
				(over all ( exists (?l - location) ( looking_at ?l )))
			)
		:effect ( and
				(at end ( kif_clean ?cup ))
			)
	)
)
