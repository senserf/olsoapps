<geometry>
	<roamer nodesize="30">
		<viscolors>red green blue</viscolors>
		<nodecolor type="peg">
			l0=(red,gray)
			l1=(green,gray)
			l2=(yellow,gray)
		</nodecolor>
		<nodecolor type="tag">lightblue</nodecolor>
		<nodecolor>aquamarine</nodecolor>
	</roamer>
	<labels>
		<sensors type="peg">
			-1Voltage,red,yellow
		</sensors>
		<sensors type="tag">
			-1=Voltage,red,yellow
			   Temp,pink,white
			   Humid,green,white
			   Moist,maroon,white
			   Infra,mediumpurple,white
		</sensors>
		<leds type="peg">
			ON, red
			XMT, blue
			RCV, orange
		</leds>
	</labels>
</geometry>
