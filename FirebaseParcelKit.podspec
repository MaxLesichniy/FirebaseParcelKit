Pod::Spec.new do |s|
  s.name         = "FirebaseParcelKit"
  s.version      = "4.0.0"
  s.summary      = "FirebaseParcelKit integrates Core Data with Google Firebase."
  s.homepage     = "http://github.com/andygeers/FirebaseParcelKit"
  s.license      = 'MIT'
  s.author       = { "Andy Geers" => "andy.geers@googlemail.com" }
  s.source       = { :git => "https://github.com/andygeers/FirebaseParcelKit.git", :tag => s.version.to_s }
  s.platform     = :ios, '7.0'
  s.source_files = 'FirebaseParcelKit/*.{h,m}'
  s.frameworks   = 'CoreData'
  s.requires_arc = true
  s.dependency 'FirebaseDatabase'

end
