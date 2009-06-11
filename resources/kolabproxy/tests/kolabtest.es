function runImapCmd( cmd )
{
  var args = ["--port", QEmu.portOffset() + 143, "--user", "autotest0@example.com", "--password", "nichtgeheim" ];
  args = args.concat( cmd );
  System.exec( "runimapcommand.py", args );
}

function testKolab( vm ) {
  QEmu.setVMConfig( vm );
  QEmu.start();

  System.exec( "create_ldap_users.py", ["-h", "localhost", "-p", QEmu.portOffset() + 389,
    "-n", "1", "--set-password", "nichtgeheim", "dc=example,dc=com", "add", "nichtgeheim" ] );

  // the server needs some time to update the user db apparently
  System.sleep( 20 );

  runImapCmd( [ "create", "INBOX/Calendar" ] );
  runImapCmd( [ "setannotation", "INBOX/Calendar", "/vendor/kolab/folder-type", "event.default" ] );
  runImapCmd( [ "append", "INBOX/Calendar", "kolabevent.mbox" ] );

  var imapResource = Resource.newInstance( "akonadi_imap_resource" );
  imapResource.setOption( "ImapServer", "localhost:42143" );
  imapResource.setOption( "UserName", "autotest0@example.com" );
  imapResource.setOption( "Password", "nichtgeheim" );
  imapResource.create();

  Test.alert( "wait" );

  XmlOperations.setXmlFile( "kolab-step1.xml" );
  XmlOperations.setRootCollections( kolabResource.identifier() );
  XmlOperations.assertEqual();

  // TODO: test changing stuff

  Test.alert( "done" );
  imapResource.destroy();
  QEmu.stop();
}

var kolabResource = Resource.newInstance( "akonadi_kolabproxy_resource" );
kolabResource.create();

testKolab( "kolabvm.conf" );
// testKolab( "dovecotvm.conf" );

kolabResource.destroy();
