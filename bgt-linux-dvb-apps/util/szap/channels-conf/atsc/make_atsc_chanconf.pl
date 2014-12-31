#!/usr/bin/perl

# Angel Li sent me this script to help in setting up a
# ~/.azap/channels.conf file automagicly.  This probbably
# isn't the final version

 use LWP;
 use HTML::Form;
 use HTTP::Cookies;
 use XML::XPath;
 use XML::XPath::XMLParser;

 #$DEBUG = 1;

 #
 # Center frequencies for NTSC channels
 #
 @ntsc = (
 	 0,  0,  57,  63,  69,  79,  85, 177, 183, 189,
 	195, 201, 207, 213, 473, 479, 485, 491, 497, 503,
 	509, 515, 521, 527, 533, 539, 545, 551, 557, 563,
 	569, 575, 581, 587, 593, 599, 605, 611, 617, 623,
 	629, 635, 641, 647, 653, 659, 665, 671, 677, 683,
 	689, 695, 701, 707, 713, 719, 725, 731, 737, 743,
 	749, 755, 761, 767, 773, 779, 785, 791, 797, 803,
 );

 $ZIPCODE = 'txtZipcode';
 $XML = 'stationXml';
 $WEBSITE = 'http://www.antennaweb.org';

 $zipCode = $ARGV[0];
 unless ($zipCode) {
 	die "Zipcode missing on the command line";
 }
 unless ($zipCode =~ /^\d\d\d\d\d$/) {
 	die "Illegal zipcode: $zipCode";
 }

 $ua = LWP::UserAgent->new;
 $ua->cookie_jar({});
 push @{$ua->requests_redirectable}, 'POST';
 $response = $ua->get($WEBSITE);
 if ($response->is_success) {
 	$form = HTML::Form->parse($response);
 	$request = $form->click("btnStart");
	$response2 = $ua->request($request);
 	if ($response2->is_success) {
 		$form2 = HTML::Form->parse($response2);
		$form2->param($ZIPCODE, $zipCode);
 		$request2 = $form2->click("btnSubmit");
 		$response3 = $ua->request($request2);
 		$form3 = HTML::Form->parse($response3);
 		$request3 = $form3->click("btnContinue");
 		$response4 = $ua->request($request3);
 		if ($response4->is_success) {
 			$form4 = HTML::Form->parse($response4);
 			$xml = $form4->value($XML);
 			$xml =~ s/%22/"/g;
 			$xml =~ s/%2c/,/g;
 			$xml =~ s/%2f/\//g;
 			$xml =~ s/%3c/</g;
 			$xml =~ s/%3d/=/g;
			$xml =~ s/%3e/>/g;
 			$xml =~ s/\+/ /g;
 			genConf($xml);
 			exit(0);
 		}
 		else {
 			print STDERR "Could not submit zipcode: $zipCode\n";
 			die $response3->status_line;
 		}
 	}
 	print STDERR "Could not reach zipcode page";
 	die $response2->status_line;
 }
 else {
 	print STDERR "Error reaching $WEBSITE\n";
 	die $response->status_line;
 }

 sub genConf {
 	my($xml) = @_;
 	my($s);
 	my($callSign);
 	my($channel);
 	my($c);
 	my($psipChannel);
 	my($freq);

	$xp = XML::XPath->new(xml => $xml);
 	foreach $s ($xp->find('//Station[BroadcastType="D"]')->get_nodelist) {
 	if ($s->find('LiveStatus')->string_value eq "1") {
 		$callSign = $s->find('CallSign')->string_value;
 		$callSign =~ s/-DT//;
 		$channel = $s->find('Channel')->string_value; # Channel to tune
		$psipChannel = $s->find('PsipChannel')->string_value;

 		if ($DEBUG) {
			print STDERR $callSign, "\t", $channel, " -> ", $psipChannel, "\n";
 		}

		$psipChannel =~ s/\.\d+$//;
		$freq = $ntsc[$channel]*1000000;
 		if ($freq) {
 			print $callSign, ":", $freq, ":8VSB:0:0\n";
 		}
 	}
 }
 }
